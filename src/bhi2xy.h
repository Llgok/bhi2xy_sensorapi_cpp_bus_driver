/*
 * @Description: BHI2xy SensorAPI 的 C++ I2C 桥接接口
 * @Author: LILYGO_L
 * @Date: 2026-07-15 23:18:00
 * @LastEditTime: 2026-07-15 23:54:00
 * @License: GPL 3.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "bhy2.h"
#include "bus/bus_guide.h"

namespace bhi2xy_sensorapi_cpp_bus_driver {

/**
 * @brief BHI2xy SensorAPI 的 C++ I2C 桥接类
 *
 * 负责把 Bosch 官方 SensorAPI 的底层读写和延时回调转接到
 * cpp_bus_driver，同时提供常用的初始化、固件启动和 FIFO 操作接口
 */
class Bhi2xy final {
 public:
  static constexpr uint32_t kDefaultI2cFrequencyHz = 400000;  // 默认I2C频率
  static constexpr uint32_t kDefaultTransferSize = 256;  // 默认单次传输长度
  static constexpr uint32_t kMinimumTransferSize = 4;    // 官方API最小传输长度
  static constexpr std::size_t kFifoWorkBufferSize = 2048;  // FIFO工作区大小

  /**
   * @brief 创建BHI2xy桥接对象
   * @param i2c_bus cpp_bus_driver的I2C总线对象
   * @param i2c_address BHI2xy器件的7位I2C地址
   */
  Bhi2xy(std::shared_ptr<cpp_bus_driver::BusI2cGuide> i2c_bus,
      uint16_t i2c_address);

  /**
   * @brief 析构对象并释放当前I2C设备
   */
  ~Bhi2xy();

  Bhi2xy(const Bhi2xy&) = delete;
  Bhi2xy& operator=(const Bhi2xy&) = delete;
  Bhi2xy(Bhi2xy&&) = delete;
  Bhi2xy& operator=(Bhi2xy&&) = delete;

  /**
   * @brief 初始化I2C设备和Bosch官方SensorAPI上下文
   * @param frequency_hz I2C通信频率
   * @param maximum_transfer_size 官方API允许的单次最大传输长度且不能小于4
   * @return 初始化成功返回 true，失败返回 false
   */
  bool Init(uint32_t frequency_hz = kDefaultI2cFrequencyHz,
      uint32_t maximum_transfer_size = kDefaultTransferSize);

  /**
   * @brief 上传固件到BHI2xy RAM并从RAM启动
   * @param firmware 固件数据指针
   * @param firmware_size 固件数据长度
   * @return 固件启动成功返回 true，失败返回 false
   */
  bool BootFromRam(const uint8_t* firmware, uint32_t firmware_size);

  /**
   * @brief 注册指定虚拟传感器的FIFO解析回调
   * @param sensor_id Bosch官方定义的虚拟传感器ID
   * @param callback FIFO数据解析回调
   * @param callback_reference 传递给回调的用户数据指针
   * @return 注册成功返回 true，失败返回 false
   */
  bool RegisterFifoCallback(uint8_t sensor_id,
      bhy2_fifo_parse_callback_t callback, void* callback_reference = nullptr);

  /**
   * @brief 更新官方API保存的虚拟传感器列表
   * @return 更新成功返回 true，失败返回 false
   */
  bool UpdateVirtualSensorList();

  /**
   * @brief 配置指定虚拟传感器的采样参数
   * @param sensor_id Bosch官方定义的虚拟传感器ID
   * @param sample_rate_hz 采样频率
   * @param report_latency_ms 数据上报延时
   * @return 配置成功返回 true，失败返回 false
   */
  bool ConfigureSensor(
      uint8_t sensor_id, float sample_rate_hz, uint32_t report_latency_ms);

  /**
   * @brief 读取FIFO并调用已注册的数据解析回调
   * @return 处理成功返回 true，失败返回 false
   */
  bool ProcessFifo();

  /**
   * @brief 释放当前I2C设备和官方API上下文
   * @param delete_bus 是否同时请求删除底层I2C总线
   * @return 释放成功返回 true，失败返回 false
   */
  bool Deinit(bool delete_bus = false);

  /**
   * @brief 获取桥接对象初始化状态
   * @return 已初始化返回 true，未初始化返回 false
   */
  bool initialized() const { return initialized_; }

  /**
   * @brief 获取固件运行状态
   * @return 固件已运行返回 true，未运行返回 false
   */
  bool firmware_running() const { return firmware_running_; }

  /**
   * @brief 获取最近一次Bosch官方API错误码
   * @return 最近一次操作的错误码
   */
  int8_t last_error() const { return last_error_; }

  /**
   * @brief 获取当前运行固件的内核版本
   * @return 固件内核版本
   */
  uint16_t kernel_version() const { return kernel_version_; }

  /**
   * @brief 获取可供Bosch官方API使用的设备上下文
   * @return 已初始化返回上下文指针，否则返回 nullptr
   */
  struct bhy2_dev* context() { return initialized_ ? &device_ : nullptr; }

  /**
   * @brief 获取只读Bosch官方API设备上下文
   * @return 已初始化返回只读上下文指针，否则返回 nullptr
   */
  const struct bhy2_dev* context() const {
    return initialized_ ? &device_ : nullptr;
  }

 private:
  /**
   * @brief Bosch官方API使用的I2C读取回调
   * @param register_address 读取起始寄存器地址
   * @param data 读取数据缓冲区
   * @param length 读取数据长度
   * @param interface_pointer 当前桥接对象指针
   * @return Bosch官方接口返回码
   */
  static BHY2_INTF_RET_TYPE ReadCallback(uint8_t register_address,
      uint8_t* data, uint32_t length, void* interface_pointer);

  /**
   * @brief Bosch官方API使用的I2C写入回调
   * @param register_address 写入起始寄存器地址
   * @param data 写入数据缓冲区
   * @param length 写入数据长度
   * @param interface_pointer 当前桥接对象指针
   * @return Bosch官方接口返回码
   */
  static BHY2_INTF_RET_TYPE WriteCallback(uint8_t register_address,
      const uint8_t* data, uint32_t length, void* interface_pointer);

  /**
   * @brief Bosch官方API使用的微秒延时回调
   * @param period_us 延时时间
   * @param interface_pointer 当前桥接对象指针
   */
  static void DelayUsCallback(uint32_t period_us, void* interface_pointer);

  /**
   * @brief 保存并检查Bosch官方API返回值
   * @param result Bosch官方API返回值
   * @return 操作成功返回 true，失败返回 false
   */
  bool CheckResult(int8_t result);

  /**
   * @brief 保存错误码并释放初始化过程中创建的I2C设备
   * @param result 需要保留的错误码
   * @return 固定返回 false
   */
  bool FailAndRelease(int8_t result);

  std::shared_ptr<cpp_bus_driver::BusI2cGuide> i2c_bus_;  // I2C总线对象
  uint16_t i2c_address_ = 0;                              // 器件I2C地址
  struct bhy2_dev device_{};  // Bosch官方API设备上下文
  std::array<uint8_t, kFifoWorkBufferSize> fifo_work_buffer_{};  // FIFO工作区
  int8_t last_error_ = BHY2_OK;    // 最近一次官方API错误码
  uint16_t kernel_version_ = 0;    // 固件内核版本
  bool bus_initialized_ = false;   // I2C设备初始化标志
  bool initialized_ = false;       // 桥接对象初始化标志
  bool firmware_running_ = false;  // 固件运行标志
};

}  // namespace bhi2xy_sensorapi_cpp_bus_driver
