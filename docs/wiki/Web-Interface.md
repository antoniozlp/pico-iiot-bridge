# Web Configuration Interface

The Pico I-IoT Bridge includes a built-in HTTP server that provides a web-based configuration interface. This allows you to configure all device parameters from any web browser on the same network, without needing any special software.

## Accessing the Web Interface

1. Connect the device to your network via Ethernet cable
2. Determine the device's IP address:
   - If **DHCP is enabled** (default), check your router's DHCP client list or use the serial console to see the assigned IP
   - If **static IP** is configured, use the IP address you previously set
3. Open a web browser and navigate to:
   ```
   http://<device-ip>/
   ```
4. The web interface loads as a single page with a sidebar for navigation

> **Tip**: The HTTP server listens on **port 80** (standard HTTP). No authentication is required.

## Interface Overview

The web interface uses a sidebar navigation layout with the following pages:

![Home Page](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/Home.png)

| Page | Description |
|---|---|
| **[Home](Web-Interface#home)** | System information (firmware version, hardware) |
| **[Network](Web-Interface:-Network-Settings)** | Ethernet network parameters (IP, subnet, gateway, DNS, DHCP) |
| **[Serial](Web-Interface:-Serial-Settings)** | UART interface parameters (baud rate, data bits, parity, stop bits, flow control) |
| **[Serial to TCP](Web-Interface:-Serial-to-TCP)** | Transparent serial-to-TCP bridge configuration |
| **[Modbus RTU](Web-Interface:-Modbus-RTU)** | Modbus RTU client settings and request configuration |
| **[Modbus TCP](Web-Interface:-Modbus-TCP)** | Modbus TCP client and server over Ethernet (remote server, listen port, requests, memory blocks) |
| **[Tag Database](Web-Interface:-Tag-Database)** | Create, view, and delete tags used for data exchange between protocols |

A **"Reboot to Apply"** button is always visible at the bottom of the sidebar. See [Applying Changes](Web-Interface:-Applying-Changes) for details on when a reboot is needed.

## Home

The Home page displays basic system information:

- **Firmware Version**: Current firmware version running on the device
- **Ethernet Interface**: The Ethernet controller in use (WIZnet W6100)

This page is read-only and serves as a quick status check.

## Configuration Workflow

The general workflow for configuring the device is:

1. **Navigate** to the desired configuration page using the sidebar
2. **Review** the current settings (loaded automatically from the device)
3. **Modify** the parameters as needed
4. **Save** by clicking the save button on that page
5. **Reboot** the device using the "Reboot to Apply" button to activate the new settings

> **Important**: Configuration changes are saved to flash memory immediately when you click Save, but most settings only take effect after a device reboot. See [Applying Changes](Web-Interface:-Applying-Changes) for details.

## Technical Details

- The HTTP server runs as a FreeRTOS task with priority 2
- It uses 2 hardware sockets from the W6100 chip for handling concurrent HTTP connections
- The web page is served as a single `index.html` page with embedded CSS and JavaScript
- Configuration is exchanged via CGI endpoints using JSON responses
- All settings are persisted to flash memory using the system configuration module
