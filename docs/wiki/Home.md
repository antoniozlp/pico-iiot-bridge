# Pico I-IoT Bridge Wiki

Welcome to the **Pico I-IoT Bridge** documentation. This wiki covers configuration, usage, and technical details of the device.

## What is the Pico I-IoT Bridge?

An industrial IoT bridge device based on the **Raspberry Pi RP2350** microcontroller and **WIZnet W6100** Ethernet chip. It acts as a versatile protocol converter and gateway for industrial automation applications, enabling seamless data exchange between legacy systems and modern IoT infrastructure.

## Quick Start

1. Connect the device to your network via Ethernet
2. The device starts with DHCP enabled by default. Find its IP address from your router or use the serial console
3. Open a web browser and navigate to `http://<device-ip>/`
4. Configure network, serial, and protocol settings through the [Web Configuration Interface](Web-Interface)

## Documentation

### Configuration

- **[Web Configuration Interface](Web-Interface)** - Browser-based setup and management
  - [Network Settings](Web-Interface:-Network-Settings) - IP, subnet, gateway, DNS, DHCP
  - [Serial Settings](Web-Interface:-Serial-Settings) - UART baud rate, data bits, parity, flow control
  - [Serial to TCP](Web-Interface:-Serial-to-TCP) - Transparent serial-to-Ethernet bridge
  - [Modbus RTU Client](Web-Interface:-Modbus-RTU#modbus-rtu-client) - Poll data from external slaves into the Tag Database
  - [Modbus RTU Server](Web-Interface:-Modbus-RTU#modbus-rtu-server) - Expose Tag Database tags as Modbus registers/coils to a master
  - [Modbus TCP Client](Web-Interface:-Modbus-TCP#modbus-tcp-client) - Poll a remote Modbus TCP server into the Tag Database
  - [Modbus TCP Server](Web-Interface:-Modbus-TCP#modbus-tcp-server) - Expose Tag Database tags to Modbus TCP clients
  - [Tag Database](Web-Interface:-Tag-Database) - Centralized variable management
  - [Applying Changes](Web-Interface:-Applying-Changes) - How to apply configuration changes

- **[CLI Reference](CLI-Reference)** - Serial console command-line interface
  - `config read/write/save` - Read, modify, and persist all device settings
  - `task-stats` - FreeRTOS task diagnostics
  - `uptime` - System uptime
  - `reboot` - Device restart

## Hardware

| Component | Details |
|---|---|
| Microcontroller | RP2350 (ARM Cortex-M33, dual-core, 150 MHz) |
| Ethernet | WIZnet W6100 (10/100 Mbps, hardware TCP/IP) |
| Serial | 2x UART with configurable parameters |
| GPIO | 2x Digital Inputs, 2x Digital Outputs |
| ADC | 3x Analog Inputs (12-bit) |
| Flash | 2 MB |
| RAM | 520 KB SRAM |

> **Note**: This project is currently developed and tested on the [W6100-EVB-Pico2](https://docs.wiznet.io/Product/Chip/Ethernet/W6100/w6100-evb-pico2) evaluation board.

## Links

- [GitHub Repository](https://github.com/antoniozlp/pico-iiot-bridge)
- [README](https://github.com/antoniozlp/pico-iiot-bridge/blob/main/README.md)
