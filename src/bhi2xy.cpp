/*
 * @Description: BHI2xy SensorAPI 的 C++ I2C 桥接实现
 * @Author: LILYGO_L
 * @Date: 2026-07-15 23:18:00
 * @LastEditTime: 2026-07-15 23:54:00
 * @License: GPL 3.0
 */
#include "bhi2xy.h"

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <utility>

namespace bhi2xy_sensorapi_cpp_bus_driver {

Bhi2xy::Bhi2xy(
    std::shared_ptr<cpp_bus_driver::I2cBusBase> i2c_bus, uint16_t i2c_address)
    : i2c_bus_(std::move(i2c_bus)), i2c_address_(i2c_address) {}

Bhi2xy::~Bhi2xy() { Deinit(false); }

bool Bhi2xy::Init(uint32_t frequency_hz, uint32_t maximum_transfer_size) {
  if (initialized_) {
    return true;
  }
  if (i2c_bus_ == nullptr) {
    last_error_ = BHY2_E_NULL_PTR;
    return false;
  }
  if (frequency_hz == 0 || i2c_address_ == 0 || i2c_address_ > 0x7F ||
      maximum_transfer_size < kMinimumTransferSize) {
    last_error_ = BHY2_E_INVALID_PARAM;
    return false;
  }
  if (!i2c_bus_->Init(frequency_hz, i2c_address_)) {
    last_error_ = BHY2_E_IO;
    return false;
  }
  bus_initialized_ = true;
  device_ = {};

  // 绑定cpp_bus_driver提供的I2C和延时回调
  if (!CheckResult(bhy2_init(BHY2_I2C_INTERFACE, ReadCallback, WriteCallback,
          DelayUsCallback, maximum_transfer_size, this, &device_))) {
    return FailAndRelease(last_error_);
  }
  if (!CheckResult(bhy2_soft_reset(&device_))) {
    return FailAndRelease(last_error_);
  }

  // 产品ID用于拦截接错地址或总线上器件不匹配的情况
  uint8_t product_id = 0;
  if (!CheckResult(bhy2_get_product_id(&product_id, &device_))) {
    return FailAndRelease(last_error_);
  }
  if (product_id != BHY2_PRODUCT_ID) {
    return FailAndRelease(BHY2_E_IO);
  }

  constexpr uint8_t kHostInterruptControl =
      BHY2_ICTL_DISABLE_STATUS_FIFO | BHY2_ICTL_DISABLE_DEBUG;
  // 保留唤醒与非唤醒FIFO并关闭未使用的状态和调试FIFO
  if (!CheckResult(
          bhy2_set_host_interrupt_ctrl(kHostInterruptControl, &device_))) {
    return FailAndRelease(last_error_);
  }
  if (!CheckResult(bhy2_set_host_intf_ctrl(0, &device_))) {
    return FailAndRelease(last_error_);
  }

  uint8_t boot_status = 0;
  if (!CheckResult(bhy2_get_boot_status(&boot_status, &device_))) {
    return FailAndRelease(last_error_);
  }
  // 主机接口就绪后才允许上传RAM固件
  if ((boot_status & BHY2_BST_HOST_INTERFACE_READY) == 0) {
    return FailAndRelease(BHY2_E_IO);
  }

  initialized_ = true;
  last_error_ = BHY2_OK;
  return true;
}

bool Bhi2xy::BootFromRam(const uint8_t* firmware, uint32_t firmware_size) {
  if (firmware_running_) {
    return true;
  }
  if (!initialized_ || firmware == nullptr || firmware_size == 0) {
    last_error_ = BHY2_E_INVALID_PARAM;
    return false;
  }
  // 固件上传和启动顺序与Bosch官方示例保持一致
  if (!CheckResult(
          bhy2_upload_firmware_to_ram(firmware, firmware_size, &device_))) {
    return false;
  }
  if (!CheckResult(bhy2_boot_from_ram(&device_))) {
    return false;
  }
  if (!CheckResult(bhy2_get_kernel_version(&kernel_version_, &device_))) {
    return false;
  }
  if (kernel_version_ == 0) {
    last_error_ = BHY2_E_IO;
    return false;
  }

  firmware_running_ = true;
  last_error_ = BHY2_OK;
  return true;
}

bool Bhi2xy::RegisterFifoCallback(uint8_t sensor_id,
    bhy2_fifo_parse_callback_t callback, void* callback_reference) {
  if (!firmware_running_ || callback == nullptr) {
    last_error_ = BHY2_E_INVALID_PARAM;
    return false;
  }
  return CheckResult(bhy2_register_fifo_parse_callback(
      sensor_id, callback, callback_reference, &device_));
}

bool Bhi2xy::UpdateVirtualSensorList() {
  if (!firmware_running_) {
    last_error_ = BHY2_E_INVALID_PARAM;
    return false;
  }
  return CheckResult(bhy2_update_virtual_sensor_list(&device_));
}

bool Bhi2xy::ConfigureSensor(
    uint8_t sensor_id, float sample_rate_hz, uint32_t report_latency_ms) {
  if (!firmware_running_ || sample_rate_hz < 0.0f) {
    last_error_ = BHY2_E_INVALID_PARAM;
    return false;
  }
  return CheckResult(bhy2_set_virt_sensor_cfg(
      sensor_id, sample_rate_hz, report_latency_ms, &device_));
}

bool Bhi2xy::ProcessFifo() {
  if (!firmware_running_) {
    last_error_ = BHY2_E_INVALID_PARAM;
    return false;
  }
  return CheckResult(bhy2_get_and_process_fifo(fifo_work_buffer_.data(),
      static_cast<uint32_t>(fifo_work_buffer_.size()), &device_));
}

bool Bhi2xy::Deinit(bool delete_bus) {
  bool result = true;
  // false只移除当前I2C设备，适用于共享总线场景
  if (bus_initialized_ && i2c_bus_ != nullptr) {
    result = i2c_bus_->Deinit(delete_bus);
  }

  device_ = {};
  fifo_work_buffer_.fill(0);
  kernel_version_ = 0;
  bus_initialized_ = false;
  initialized_ = false;
  firmware_running_ = false;
  last_error_ = result ? BHY2_OK : BHY2_E_IO;
  return result;
}

BHY2_INTF_RET_TYPE Bhi2xy::ReadCallback(uint8_t register_address, uint8_t* data,
    uint32_t length, void* interface_pointer) {
  auto* instance = static_cast<Bhi2xy*>(interface_pointer);
  if (instance == nullptr || instance->i2c_bus_ == nullptr || data == nullptr) {
    return BHY2_E_NULL_PTR;
  }
  return instance->i2c_bus_->WriteRead(&register_address, 1, data, length)
             ? BHY2_INTF_RET_SUCCESS
             : BHY2_E_IO;
}

BHY2_INTF_RET_TYPE Bhi2xy::WriteCallback(uint8_t register_address,
    const uint8_t* data, uint32_t length, void* interface_pointer) {
  auto* instance = static_cast<Bhi2xy*>(interface_pointer);
  if (instance == nullptr || instance->i2c_bus_ == nullptr || data == nullptr) {
    return BHY2_E_NULL_PTR;
  }
  if (length >= std::numeric_limits<size_t>::max()) {
    return BHY2_E_IO;
  }
  const size_t packet_length = static_cast<size_t>(length) + 1;
  std::array<uint8_t, 128> local_packet{};
  std::unique_ptr<uint8_t[]> heap_packet;
  if (packet_length > local_packet.size()) {
    heap_packet.reset(new (std::nothrow) uint8_t[packet_length]);
    if (heap_packet == nullptr) {
      return BHY2_E_IO;
    }
  }
  uint8_t* packet =
      heap_packet != nullptr ? heap_packet.get() : local_packet.data();
  packet[0] = register_address;
  if (length != 0) {
    std::memcpy(packet + 1, data, length);
  }
  return instance->i2c_bus_->Write(packet, packet_length)
             ? BHY2_INTF_RET_SUCCESS
             : BHY2_E_IO;
}

void Bhi2xy::DelayUsCallback(uint32_t period_us, void* interface_pointer) {
  auto* instance = static_cast<Bhi2xy*>(interface_pointer);
  if (instance != nullptr && instance->i2c_bus_ != nullptr) {
    instance->i2c_bus_->DelayUs(period_us);
  }
}

bool Bhi2xy::CheckResult(int8_t result) {
  last_error_ = result;
  return result == BHY2_OK;
}

bool Bhi2xy::FailAndRelease(int8_t result) {
  Deinit(false);
  last_error_ = result;
  return false;
}

}  // namespace bhi2xy_sensorapi_cpp_bus_driver
