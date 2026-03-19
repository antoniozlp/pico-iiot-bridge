# Pico I-IoT Bridge

An industrial IoT bridge device based on the Raspberry Pi RP2350 microcontroller and WIZnet W6100 Ethernet chip. This device acts as a versatile protocol converter and gateway for industrial automation applications.

## Overview

The Pico I-IoT Bridge is designed to bridge different industrial communication protocols, enabling seamless data exchange between legacy systems and modern IoT infrastructure. Built on a robust FreeRTOS foundation, it provides real-time performance, configuration flexibility, and reliable operation in industrial environments.

## Documentation

Detailed user and configuration documentation is available in the GitHub Wiki:

- [Project Wiki](https://github.com/antoniozlp/pico-iiot-bridge/wiki)
- [Web Interface Overview](https://github.com/antoniozlp/pico-iiot-bridge/wiki/Web-Interface)
- [Modbus RTU Client and Server](https://github.com/antoniozlp/pico-iiot-bridge/wiki/Web-Interface:-Modbus-RTU)
- [CLI Reference](https://github.com/antoniozlp/pico-iiot-bridge/wiki/CLI-Reference)

## Key Features

### Current Capabilities

- **FreeRTOS RTOS**: Dual-core task scheduling with SMP support on RP2350
- **Ethernet Connectivity**: W6100 chip with hardware TCP/IP stack
- **Web Configuration Interface**: HTTP server with responsive web UI for easy setup
- **CLI Configuration**: Serial command-line interface for advanced configuration
- **Persistent Configuration**: Flash-based storage for network, serial, Modbus, and TCP settings
- **Tag Database**: Centralized variable storage shared across tasks
- **Modbus RTU Client**: Poll external slave devices and map values to tags
- **Modbus RTU Server**: Expose tags as Modbus registers/coils to an external master
- **Serial to TCP Bridge**: Transparent serial-to-Ethernet gateway

### Planned Features

- **Modbus TCP to Modbus RTU Converter**: Protocol bridge for Modbus networks
- **Modbus TCP Client/Server**: Native Modbus TCP communication support
- **Remote I/O (RTU)**: Analog and digital I/O over Ethernet
- **MQTT Client**: Publish and subscribe industrial data to IoT platforms
- **OPC UA Support**: Interoperability with industrial SCADA and MES systems
- **And more**: Expandable architecture for additional protocols

## Hardware

### Microcontroller
- **RP2350**: ARM Cortex-M33 dual-core processor
- **Clock Speed**: 150 MHz per core
- **RAM**: 520 KB SRAM
- **Flash**: 2 MB

### Ethernet Controller
- **W6100**: WIZnet's hardwired TCP/IP chip
- **Supported Protocols**: TCP, UDP, IPv4, IPv6
- **Hardware Sockets**: 8 independent hardware sockets

> **Note**: This project is currently developed and tested on the [W6100-EVB-Pico2](https://docs.wiznet.io/Product/Chip/Ethernet/W6100/w6100-evb-pico2) evaluation board.


### Interfaces
- **Ethernet**: 10/100 Mbps via W6100
- **Serial**: 2x UART with configurable parameters
- **GPIO**: 2x Digital Inputs, 2x Digital Outputs
- **ADC**: 3x Analog Inputs (12-bit)
- **I2C**: Available for expansion
- **SPI**: Used for W6100 communication

## Software Architecture

### Core Components

#### FreeRTOS Real-Time Operating System
- Preemptive multitasking with priority-based scheduling
- Dual-core SMP (Symmetric Multi-Processing) support
- Task stack overflow detection

#### Configuration Management
- **Network Configuration**: IP, subnet, gateway, DNS, MAC address
- **Serial Configuration**: Baud rate, data bits, parity, stop bits, flow control
- **TCP Configuration**: Port, timeout
- **Flash Persistence**: Automatic save/load with version control
- **Dual-Core Safe**: Coordinated flash writes across both CPU cores

#### Tag Database
- **Centralized Variable Storage**: Named tag system for sharing data between tasks
- **Non-blocking Writes**: Queue-based communication for producer tasks
- **Quality Indicators**: Track data validity (GOOD, BAD, UNCERTAIN)
- **Type Support**: BOOL, UINT8, UINT16, UINT32, INT16, INT32, FLOAT
- **Minimal Overhead**: ~6 KB RAM for 128 tags
- **Thread-Safe**: FreeRTOS mutex and queue protection

#### Communication Interfaces
1. **HTTP Server**
   - Web UI for user-friendly management

2. **CLI (Command Line Interface)**
   - FreeRTOS-Plus-CLI integration

3. **WIZnet Driver**
   - Hardware TCP/IP offload
   - SPI communication with DMA support
   - Network utilities and socket management


## Getting Started

### Prerequisites 

- Pico SDK 2.2.0 or later
- CMake 3.13
- ARM GCC toolchain (13.3 Rel1 or later)
- FreeRTOS Kernel (included as submodule)

### Building

```bash
# Clone the repository
git clone <repository-url>
cd pico-iiot-bridge

# Initialize submodules (FreeRTOS Kernel)
git submodule update --init --recursive
```

### Compiling and Flashing

This project uses the **Raspberry Pi Pico VS Code Extension** for building and flashing firmware. 

1. Install the [Raspberry Pi Pico VS Code Extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico)
2. Import this project using the extension
3. Use the extension's build and flash tools to compile and upload firmware to your board

For detailed instructions on setting up the development environment, refer to the official [Getting Started with Raspberry Pi Pico](https://pip-assets.raspberrypi.com/categories/610-raspberry-pi-pico/documents/RP-008276-DS-1-getting-started-with-pico.pdf) guide.


## Tag Database Usage

The tag database provides a centralized way to share variables between tasks (e.g., Modbus, MQTT, HTTP server).

### Quick Example

```c
// Create tags at initialization
tag_handle_t temp_handle = tag_db_create("TEMP_SENSOR_01", TAG_TYPE_FLOAT);

// Producer task writes value
tag_value_t value;
value.float_val = 25.3;
tag_db_write(temp_handle, value, TAG_QUALITY_GOOD);

// Consumer task reads value
tag_db_read(temp_handle, &value, NULL, NULL);
float temperature = value.float_val;
```

For detailed examples, see `src/tag-database/tag_database_example.c` and `VARIABLE_DATABASE_PROPOSAL_V3.md`.

## Development Roadmap

- [x] Tag Database for variable sharing
- [x] HTTP server integration with tag database
- [x] Modbus RTU client
- [x] Modbus RTU server
- [x] Serial to TCP transparent bridge
- [ ] Modbus TCP to RTU converter
- [ ] Modbus TCP client/server
- [ ] Remote I/O functionality
- [ ] MQTT client support
- [ ] OPC UA support
