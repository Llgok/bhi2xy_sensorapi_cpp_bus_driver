<h1 align="center">bhi2xy_sensorapi_cpp_bus_driver</h1>

## **[English](./README.md) | 中文**

[![Release](https://img.shields.io/github/v/release/Llgok/bhi2xy_sensorapi_cpp_bus_driver?style=flat-square)](https://github.com/Llgok/bhi2xy_sensorapi_cpp_bus_driver/releases)
[![License](https://img.shields.io/github/license/Llgok/bhi2xy_sensorapi_cpp_bus_driver?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.3%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)

`bhi2xy_sensorapi_cpp_bus_driver` 是 Bosch Sensortec 官方 BHI2xy SensorAPI
与 [`cpp_bus_driver`](https://github.com/Llgok/cpp_bus_driver) 之间的
ESP-IDF 桥接组件。它使用 C++ 对象提供官方 API 所需的 I2C 传输和延时服务，
并保持随附的官方源码不变。

## 目录

- [特性](#特性)
- [支持框架](#支持框架)
- [快速开始](#快速开始)
- [注意事项](#注意事项)

## 特性

- 使用 `cpp_bus_driver` 提供 I2C 访问和微秒延时能力。
- 提供设备初始化、RAM 固件启动、FIFO 处理和虚拟传感器配置接口。
- 通过 `context()` 公开官方 `bhy2_dev`，保留完整 SensorAPI 访问能力。
- Bosch Sensortec 官方源码原样保存在 `third_party` 中。
- 支持通过 `cpp_bus_driver` 总线对象共享 ESP-IDF I2C 总线。

组件的实际支持范围以公开头文件、构建配置和随附的官方源码为准。

## 支持框架

| 框架 | 状态 | 说明 |
| --- | --- | --- |
| ESP-IDF | 支持 | 通过 `cpp_bus_driver` 提供底层 I2C 访问 |

## 快速开始

### 集成方式

#### 作为 ESP-IDF component 使用

把 `cpp_bus_driver` 与本仓库放入工程的 `components` 目录：

```text
your_project/
├── components/
│   ├── cpp_bus_driver/
│   └── bhi2xy_sensorapi_cpp_bus_driver/
├── main/
└── CMakeLists.txt
```

克隆命令：

```bash
git clone https://github.com/Llgok/cpp_bus_driver.git
git clone --recursive https://github.com/Llgok/bhi2xy_sensorapi_cpp_bus_driver.git
```

克隆本仓库时请同时拉取子模块，如果克隆时没有使用 `--recursive`，请手动初始化子模块：

```bash
git submodule update --init --recursive
```

然后包含统一入口头文件：

```cpp
#include "bhi2xy_sensorapi_cpp_bus_driver_library.h"
```

#### 作为 Git submodule 使用

```bash
git submodule add https://github.com/Llgok/cpp_bus_driver.git
git submodule add https://github.com/Llgok/bhi2xy_sensorapi_cpp_bus_driver.git
git submodule update --init --recursive
```

子模块目录可根据工程结构调整，只需确保 ESP-IDF 能找到两个组件。

### 创建 I2C 总线和驱动对象

```cpp
#include <memory>

#include "bhi2xy_sensorapi_cpp_bus_driver_library.h"
#include "cpp_bus_driver_library.h"

auto i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c1>(
    sda_pin, scl_pin, I2C_NUM_0);

bhi2xy_sensorapi_cpp_bus_driver::Bhi2xy sensor(i2c_bus, 0x28);
if (!sensor.Init()) {
  return;
}
```

I2C 端口、引脚、地址和通信频率应根据实际硬件设置。

### 固件和传感器生命周期

```cpp
if (!sensor.BootFromRam(firmware, firmware_size)) {
  return;
}

sensor.RegisterFifoCallback(sensor_id, callback);
sensor.ProcessFifo();
sensor.UpdateVirtualSensorList();
sensor.ConfigureSensor(sensor_id, 100.0f, 0);

while (sensor.ProcessFifo()) {
  // 解析后的数据会传递给已注册的回调。
}
```

需要使用与目标 BHI2xy 器件匹配的固件。应用可以包含
`third_party/BHI2xy_SensorAPI/firmware` 目录内随附的官方固件头文件。

## 注意事项

- `context()` 返回已初始化的官方 `bhy2_dev`，可用于调用未封装的 API。
- `Deinit(false)` 仅释放当前对象使用的 I2C 设备。
- `Deinit(true)` 还会请求删除底层 I2C 总线，仅应在当前对象独占总线时使用。
- 桥接层默认不提供多任务并发保护。
- 桥接代码采用 GPL-3.0，Bosch Sensortec 官方文件保留其原始许可条款。

详细 API 请查看公开头文件和 BHI2xy SensorAPI 官方文档。
