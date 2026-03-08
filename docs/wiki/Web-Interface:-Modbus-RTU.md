# Modbus RTU Settings

The Modbus RTU page configures the built-in Modbus RTU client (master). This allows the Pico I-IoT Bridge to poll data from Modbus RTU slave devices connected via serial and store the results in the [Tag Database](Web-Interface:-Tag-Database).

![Modbus RTU Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/ModbusRTU.png)

## Modbus RTU Client Settings

The top section configures global Modbus RTU client parameters:

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable Modbus RTU** | Enable or disable the Modbus RTU client | Enabled / Disabled |
| **Serial Port** | Which UART to use for Modbus communication | UART0 (Console), UART1 (Bridge) |

Click **"Save Client Settings"** to save these parameters.

> **Recommendation**: Use **UART1 (Bridge)** for Modbus RTU communication, leaving UART0 available for the CLI console. Make sure the serial port settings (baud rate, parity, etc.) on the [Serial Settings](Web-Interface:-Serial-Settings) page match your Modbus slave devices.

## Modbus Requests

Below the client settings, you can configure up to **10 independent Modbus requests** (Request 0 through Request 9). Each request defines a polling operation that the Modbus client executes cyclically. Select a request using the numbered tab buttons.

### Request Parameters

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable** | Enable or disable this request | Enabled / Disabled |
| **Slave Address** | Modbus slave address to query | 1 - 247 |
| **Data Type** | Type of Modbus register/coil to access | See table below |
| **Operation** | Read or Write operation | Read, Write |
| **Start Address** | Starting register/coil address | 0 - 65535 |
| **Count** | Number of registers/coils to read/write | 1 - 10 |
| **32-bit Encoding** | Byte order for 32-bit values spanning 2 registers | See table below |

### Data Types

| Value | Name | Description |
|---|---|---|
| Coil | R/W, 1-bit | Discrete output (Function codes 01/05/15) |
| Discrete Input | RO, 1-bit | Discrete input (Function code 02) |
| Input Register | RO, 16-bit | Input register (Function code 04) |
| Holding Register | R/W, 16-bit | Holding register (Function codes 03/06/16) |

### 32-bit Encoding Options

When reading 32-bit values (e.g., FLOAT, UINT32) that span two consecutive 16-bit registers, the byte order matters. Different Modbus devices use different conventions:

| Encoding | Description | Byte Order |
|---|---|---|
| **ABCD** | Big-endian (most common) | High word first, high byte first |
| **BADC** | Word swap | High word first, low byte first |
| **CDAB** | Byte swap | Low word first, high byte first |
| **DCBA** | Word + byte swap | Low word first, low byte first |

> **Tip**: If you're reading floating-point values and getting incorrect results, try changing the encoding. ABCD is the most common convention, but some devices (especially older ones) may use a different order.

## Tag Mapping

Each request includes a **Tag Mapping** section that links individual registers or coils to tags in the [Tag Database](Web-Interface:-Tag-Database). This is the mechanism by which Modbus data becomes available to other parts of the system.

- **Reg/Coil 0** through **Reg/Coil 9** correspond to the registers starting at **Start Address**
- Select a tag from the dropdown to map that register to it, or choose **"Not Mapped"** to skip
- The dropdown shows all available tags with their data types (e.g., `TEMP_S1 (FLOAT)`)
- Tags must be created first on the [Tag Database](Web-Interface:-Tag-Database) page before they appear here

### Example Mapping

If you have a request configured as:
- **Start Address**: 0
- **Count**: 4
- **Data Type**: Holding Register

Then:
- **Reg/Coil 0** = Register 0 → mapped to `TEMP_S1`
- **Reg/Coil 1** = Register 1 → not mapped
- **Reg/Coil 2** = Register 2 → mapped to `PRESURE_S1`
- **Reg/Coil 3** = Register 3 → not mapped

Click **"Save Request N"** to save the request configuration.

## Saving Settings

1. Configure the **Client Settings** and click **"Save Client Settings"**
2. Select each request tab, configure its parameters and tag mappings, then click **"Save Request N"**
3. Click **"Reboot to Apply"** in the sidebar to start the Modbus client with the new configuration

## CGI Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_modbus.cgi` | GET | Returns full Modbus configuration (client + all requests) as JSON |
| `set_modbus_client.cgi` | POST | Updates client settings (enable, serial port) |
| `set_modbus_datapoint.cgi` | POST | Updates a single request configuration |

### GET Response Example

```json
{
  "enable": 1,
  "serial_id": 1,
  "requests": [
    {
      "enabled": 1,
      "slave_address": 1,
      "data_type": 3,
      "operation": 0,
      "start_address": 0,
      "count": 4,
      "encoding": 0,
      "tag_handles": [0, 255, 1, 255, 255, 255, 255, 255, 255, 255]
    }
  ]
}
```

In `tag_handles`, the value `255` means **Not Mapped**. Other values are tag handle indices from the Tag Database.

### POST Parameters (set_modbus_client.cgi)

| Parameter | Description |
|---|---|
| `enable` | `1` = enabled, `0` = disabled |
| `serial_id` | Serial port: `0` or `1` |

### POST Parameters (set_modbus_datapoint.cgi)

| Parameter | Description |
|---|---|
| `dp_idx` | Request index (0 - 9) |
| `enabled` | `1` = enabled, `0` = disabled |
| `slave_address` | Slave address (1 - 247) |
| `data_type` | 0 = Coil, 1 = Discrete Input, 2 = Input Register, 3 = Holding Register |
| `operation` | 0 = Read, 1 = Write |
| `start_address` | Start address (0 - 65535) |
| `count` | Register/coil count (1 - 10) |
| `encoding` | 0 = ABCD, 1 = BADC, 2 = CDAB, 3 = DCBA |
| `tag0` - `tag9` | Tag handle for each register (0 - 127, or 255 = not mapped) |
