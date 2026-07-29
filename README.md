<h1 align="center">bhi2xy_sensorapi_cpp_bus_driver</h1>

## **English | [Chinese](./README_CN.md)**

[![Release](https://img.shields.io/github/v/release/Llgok/bhi2xy_sensorapi_cpp_bus_driver?style=flat-square)](https://github.com/Llgok/bhi2xy_sensorapi_cpp_bus_driver/releases)
[![License](https://img.shields.io/github/license/Llgok/bhi2xy_sensorapi_cpp_bus_driver?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.3%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)

`bhi2xy_sensorapi_cpp_bus_driver` is an ESP-IDF bridge between Bosch
Sensortec's official BHI2xy SensorAPI and
[`cpp_bus_driver`](https://github.com/Llgok/cpp_bus_driver). It supplies the
official API with C++ managed I2C transport and delay services while keeping
the bundled official sources unchanged.

## Table of Contents

- [Features](#features)
- [Supported Frameworks](#supported-frameworks)
- [Quick Start](#quick-start)
- [Notes](#notes)

## Features

- Uses `cpp_bus_driver` for I2C access and microsecond delays.
- Provides device initialization, RAM firmware boot, FIFO processing, and
  virtual sensor configuration helpers.
- Exposes the official `bhy2_dev` through `context()` for complete SensorAPI
  access.
- Keeps Bosch Sensortec's official sources in `third_party` unchanged.
- Supports shared ESP-IDF I2C buses through `cpp_bus_driver` bus objects.

The public headers, build configuration, and bundled official sources are the
source of truth for currently supported functionality.

## Supported Frameworks

| Framework | Status | Description |
| --- | --- | --- |
| ESP-IDF | Supported | Uses `cpp_bus_driver` for low-level I2C access |

## Quick Start

### Integration

#### Use as ESP-IDF Components

Place `cpp_bus_driver` and this repository in the project's `components`
directory:

```text
your_project/
├── components/
│   ├── cpp_bus_driver/
│   └── bhi2xy_sensorapi_cpp_bus_driver/
├── main/
└── CMakeLists.txt
```

Clone commands:

```bash
git clone https://github.com/Llgok/cpp_bus_driver.git
git clone --recursive https://github.com/Llgok/bhi2xy_sensorapi_cpp_bus_driver.git
```

When cloning this repository, please also fetch its submodules. If you did not use `--recursive` when cloning, initialize the submodules manually:

```bash
git submodule update --init --recursive
```

Then include the unified entry header:

```cpp
#include "bhi2xy_sensorapi_cpp_bus_driver_library.h"
```

#### Use as Git Submodules

```bash
git submodule add https://github.com/Llgok/cpp_bus_driver.git
git submodule add https://github.com/Llgok/bhi2xy_sensorapi_cpp_bus_driver.git
git submodule update --init --recursive
```

Submodule directories may be customized as long as ESP-IDF can discover both
components.

### Create the I2C Bus and Driver

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

Select the I2C port, pins, address, and communication frequency for the target
hardware.

### Firmware and Sensor Lifecycle

```cpp
if (!sensor.BootFromRam(firmware, firmware_size)) {
  return;
}

sensor.RegisterFifoCallback(sensor_id, callback);
sensor.ProcessFifo();
sensor.UpdateVirtualSensorList();
sensor.ConfigureSensor(sensor_id, 100.0f, 0);

while (sensor.ProcessFifo()) {
  // Parsed data is delivered to the registered callback.
}
```

Use firmware matching the target BHI2xy device. The official firmware headers
bundled under `third_party/BHI2xy_SensorAPI/firmware` can be included by the
application.

## Notes

- `context()` returns the initialized official `bhy2_dev` for APIs that are
  not wrapped by the convenience interface.
- `Deinit(false)` releases only the current I2C device.
- `Deinit(true)` also requests deletion of the underlying I2C bus; use it only
  when the current object exclusively owns that bus.
- The bridge does not add multi-task synchronization.
- The bridge code is GPL-3.0. Official Bosch Sensortec files retain their
  original license terms.

Refer to the public headers and the official BHI2xy SensorAPI documentation
for detailed APIs.
