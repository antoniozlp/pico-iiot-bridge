# Serial Settings

The Serial Settings page allows you to configure the two UART interfaces available on the Pico I-IoT Bridge.

![Serial Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/Serial.png)

## Serial Ports

The device has two independent UART interfaces, selectable via tab buttons at the top of the page:

| Port | Label | Typical Use |
|---|---|---|
| **Serial 0** | Console | CLI debug console, logging output |
| **Serial 1** | Bridge | Serial-to-TCP bridge, Modbus RTU communication |

Each port is configured independently with its own set of parameters.

## Parameters

| Parameter | Description | Valid Values |
|---|---|---|
| **Baud Rate** | Communication speed | 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 |
| **Data Bits** | Number of data bits per frame | 5, 6, 7, 8 |
| **Parity** | Error detection parity bit | None, Even, Odd |
| **Stop Bits** | Number of stop bits per frame | 1, 2 |
| **CTS Flow Control** | Clear To Send hardware flow control | Enabled / Disabled (checkbox) |
| **RTS Flow Control** | Request To Send hardware flow control | Enabled / Disabled (checkbox) |

## Typical Configurations

### Standard Console (Serial 0)
- Baud Rate: **115200**
- Data Bits: **8**
- Parity: **None**
- Stop Bits: **1**
- Flow Control: **Disabled**

### Modbus RTU (Serial 1)
- Baud Rate: **9600** or **19200** (match your Modbus devices)
- Data Bits: **8**
- Parity: **None** or **Even** (Modbus standard recommends Even)
- Stop Bits: **1** (or 2 if parity is None, per Modbus spec)
- Flow Control: **Disabled** (RS-485 typically does not use CTS/RTS)

## Saving Serial Settings

1. Select the serial port tab (**Serial 0** or **Serial 1**)
2. Modify the desired parameters
3. Click **"Save Serial 0"** or **"Save Serial 1"** depending on the selected port
4. Click **"Reboot to Apply"** in the sidebar to activate the new serial configuration

> **Note**: Changing Serial 0 settings will affect the CLI console. Make sure your terminal application matches the new settings after reboot, or you will lose console access.

## CGI Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_serial.cgi` | GET | Returns configuration for both serial ports as JSON |
| `set_serial.cgi` | POST | Updates configuration for one serial port |

### GET Response Example

```json
{
  "serial0": {
    "baud": 115200,
    "databits": 8,
    "parity": "none",
    "stopbits": 1,
    "flowcts": 0,
    "flowrts": 0
  },
  "serial1": {
    "baud": 9600,
    "databits": 8,
    "parity": "even",
    "stopbits": 1,
    "flowcts": 0,
    "flowrts": 0
  }
}
```

### POST Parameters

| Parameter | Description |
|---|---|
| `uart` | Port index: `0` or `1` |
| `baud` | Baud rate (9600 - 921600) |
| `databits` | Data bits (5 - 8) |
| `parity` | `none`, `even`, or `odd` |
| `stopbits` | `1` or `2` |
| `flowcts` | `1` = enabled, `0` = disabled |
| `flowrts` | `1` = enabled, `0` = disabled |
