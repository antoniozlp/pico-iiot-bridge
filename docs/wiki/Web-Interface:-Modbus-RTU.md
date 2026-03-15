# Modbus RTU Settings

The Modbus RTU section of the web interface covers two independent features:

- **Modbus RTU Client** — polls data from external slave devices and writes it into the Tag Database
- **Modbus RTU Server** — exposes Tag Database tags as Modbus registers/coils so an external master can read and write them

Both features run as separate FreeRTOS tasks and can use the same or different serial ports. Serial port settings (baud rate, parity, etc.) are configured on the [Serial Settings](Web-Interface:-Serial-Settings) page.

---

## Modbus RTU Client

![Modbus RTU Client Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/ModbusRTU.png)

### Client Settings

The top section configures global Modbus RTU client parameters:

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable Modbus RTU** | Enable or disable the Modbus RTU client | Enabled / Disabled |
| **Serial Port** | Which UART to use for Modbus communication | UART0 (Console), UART1 (Bridge) |

Click **"Save Client Settings"** to save these parameters.

> **Recommendation**: Use **UART1 (Bridge)** for Modbus RTU communication, leaving UART0 available for the CLI console. Make sure the serial port settings (baud rate, parity, etc.) on the [Serial Settings](Web-Interface:-Serial-Settings) page match your Modbus slave devices.

### Modbus Requests

Below the client settings, you can configure up to **10 independent Modbus requests** (Request 0 through Request 9). Each request defines a polling operation that the Modbus client executes cyclically. Select a request using the numbered tab buttons.

#### Request Parameters

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable** | Enable or disable this request | Enabled / Disabled |
| **Slave Address** | Modbus slave address to query | 1 - 247 |
| **Data Type** | Type of Modbus register/coil to access | See table below |
| **Operation** | Read or Write operation | Read, Write |
| **Start Address** | Starting register/coil address | 0 - 65535 |
| **Count** | Number of registers/coils to read/write | 1 - 10 |
| **32-bit Encoding** | Byte order for 32-bit values spanning 2 registers | See table below |

#### Data Types

| Value | Name | Description |
|---|---|---|
| Coil | R/W, 1-bit | Discrete output (Function codes 01/05/15) |
| Discrete Input | RO, 1-bit | Discrete input (Function code 02) |
| Input Register | RO, 16-bit | Input register (Function code 04) |
| Holding Register | R/W, 16-bit | Holding register (Function codes 03/06/16) |

#### 32-bit Encoding Options

When reading 32-bit values (e.g., FLOAT, UINT32) that span two consecutive 16-bit registers, the byte order matters. Different Modbus devices use different conventions:

| Encoding | Description | Byte Order |
|---|---|---|
| **ABCD** | Big-endian (most common) | High word first, high byte first |
| **BADC** | Word swap | High word first, low byte first |
| **CDAB** | Byte swap | Low word first, high byte first |
| **DCBA** | Word + byte swap | Low word first, low byte first |

> **Tip**: If you're reading floating-point values and getting incorrect results, try changing the encoding. ABCD is the most common convention, but some devices (especially older ones) may use a different order.

### Tag Mapping (Client)

Each request includes a **Tag Mapping** section that links individual registers or coils to tags in the [Tag Database](Web-Interface:-Tag-Database). This is the mechanism by which Modbus data becomes available to other parts of the system.

- **Reg/Coil 0** through **Reg/Coil 9** correspond to the registers starting at **Start Address**
- Select a tag from the dropdown to map that register to it, or choose **"Not Mapped"** to skip
- The dropdown shows all available tags with their data types (e.g., `TEMP_S1 (FLOAT)`)
- Tags must be created first on the [Tag Database](Web-Interface:-Tag-Database) page before they appear here

#### Example Mapping

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

### Saving Client Settings

1. Configure the **Client Settings** and click **"Save Client Settings"**
2. Select each request tab, configure its parameters and tag mappings, then click **"Save Request N"**
3. Click **"Reboot to Apply"** in the sidebar to start the Modbus client with the new configuration

---

## Modbus RTU Server

The Modbus RTU Server turns the Pico I-IoT Bridge into a **Modbus RTU slave** (server). An external Modbus master (PLC, SCADA system, HMI, etc.) can connect via serial and read or write Tag Database variables directly over Modbus RTU.

![Modbus RTU Server Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/ModbusRTUServer.png)

### Server Settings

The top section configures the global server parameters:

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable Modbus RTU Server** | Enable or disable the server task | Enabled / Disabled |
| **Serial Port** | Which UART to use | UART0 (Console), UART1 (Bridge) |
| **Server RTU Address** | The Modbus slave address this device responds to | 1 - 247 |

Click **"Save Server Settings"** to save these parameters.

> **Note**: The server and client can share the same UART if you are running them on the same RS-485 bus with different device addresses. In most deployments, assign a unique serial port to each feature to avoid collisions.

### Server Memory Map (Memory Blocks)

Below the server settings, you configure up to **10 memory blocks** (Block 0 through Block 9). Each memory block defines a contiguous range of Modbus addresses that the server will respond to. An incoming master request must fall entirely within a single block — requests that span two blocks return a Modbus exception `ILLEGAL_DATA_ADDRESS`.

Select a block using the numbered tab buttons.

#### Memory Block Parameters

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable** | Enable or disable this memory block | Enabled / Disabled |
| **Data Type** | The Modbus address space this block occupies | See table below |
| **Allow Master Writes** | Whether the master can write to this block | No (read-only), Yes (read/write) |
| **32-bit Encoding** | Byte/word order for 32-bit values | ABCD, BADC, CDAB, DCBA |
| **Start Address** | First Modbus address of this block | 0 - 65535 |
| **Count** | Number of registers or coils in this block | 1 - 10 |

#### Data Types

| Value | Name | Supported Function Codes |
|---|---|---|
| Coil | R/W, 1-bit | FC01 (Read), FC05 (Write Single), FC15 (Write Multiple) |
| Discrete Input | RO, 1-bit | FC02 (Read) |
| Input Register | RO, 16-bit | FC04 (Read) |
| Holding Register | R/W, 16-bit | FC03 (Read), FC16 (Write Multiple) |

> **Write access**: For Coil and Holding Register blocks, write function codes are only accepted if **Allow Master Writes** is set to **Yes (read/write)**. If a master attempts to write to a read-only block, the server returns a Modbus exception `ILLEGAL_FUNCTION`.

#### 32-bit Encoding

The same encoding options as the client apply here (ABCD, BADC, CDAB, DCBA). This controls how 32-bit tag values (FLOAT, UINT32, INT32) are packed into two consecutive 16-bit registers when the master reads or writes them.

### Tag Mapping (Server)

Each memory block includes a **Tag Mapping** section that links Modbus addresses to tags in the Tag Database.

- **Reg/Coil 0** through **Reg/Coil 9** map to consecutive addresses starting from **Start Address**
- Select a tag from the dropdown, or choose **"Not Mapped"** to leave that address unmapped
- When a master reads an unmapped address, the value returned is `0`
- When a master writes to an unmapped address, the write is silently discarded

Tags must be created on the [Tag Database](Web-Interface:-Tag-Database) page before they appear in the dropdown.

#### Example

A block configured as:
- **Data Type**: Holding Register
- **Start Address**: 100
- **Count**: 6
- **Allow Master Writes**: Yes

With tag mapping:
- Reg/Coil 0 → `VAR01` (address 100)
- Reg/Coil 1 → `VAR02` (address 101)
- Reg/Coil 2 → `VAR03` (address 102)
- Reg/Coil 3 → Not Mapped (address 103)
- Reg/Coil 4 → `VAR04` (address 104)
- Reg/Coil 5 → Not Mapped (address 105)

A master reading FC03 addresses 100–105 receives live values from `VAR01`, `VAR02`, `VAR03`, `0`, `VAR04`, `0`. A master writing FC16 to addresses 100–102 updates `VAR01`, `VAR02`, `VAR03` in the Tag Database.

Click **"Save Memory Block N"** to save the block configuration.

### Saving Server Settings

1. Configure the **Server Settings** and click **"Save Server Settings"**
2. Select each memory block tab, configure its parameters and tag mappings, then click **"Save Memory Block N"**
3. Click **"Reboot to Apply"** in the sidebar to start the server with the new configuration

---

## CGI Endpoints

### Client Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_modbus.cgi` | GET | Returns full Modbus client configuration (settings + all requests) as JSON |
| `set_modbus_client.cgi` | POST | Updates client settings (enable, serial port) |
| `set_modbus_datapoint.cgi` | POST | Updates a single request configuration |

#### GET Response Example (`get_modbus.cgi`)

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

In `tag_handles`, `255` means **Not Mapped**. Other values are tag handle indices from the Tag Database.

#### POST Parameters (`set_modbus_client.cgi`)

| Parameter | Description |
|---|---|
| `enable` | `1` = enabled, `0` = disabled |
| `serial_id` | Serial port: `0` or `1` |

#### POST Parameters (`set_modbus_datapoint.cgi`)

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

---

### Server Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_modbus_server.cgi` | GET | Returns full Modbus server configuration (settings + all memory blocks) as JSON |
| `set_modbus_server.cgi` | POST | Updates server settings (enable, serial port, server address) |
| `set_modbus_server_memory_block.cgi` | POST | Updates a single memory block configuration |

#### GET Response Example (`get_modbus_server.cgi`)

```json
{
  "enable": 1,
  "serial_id": 1,
  "server_address": 14,
  "memory_blocks": [
    {
      "enabled": 1,
      "data_type": 3,
      "writable": 1,
      "start_address": 0,
      "count": 6,
      "encoding": 0,
      "tag_handles": [0, 1, 2, 255, 3, 255, 255, 255, 255, 255]
    }
  ]
}
```

In `tag_handles`, `255` means **Not Mapped**.

#### POST Parameters (`set_modbus_server.cgi`)

| Parameter | Description |
|---|---|
| `enable` | `1` = enabled, `0` = disabled |
| `serial_id` | Serial port: `0` or `1` |
| `server_address` | RTU slave address (1 - 247) |

#### POST Parameters (`set_modbus_server_memory_block.cgi`)

| Parameter | Description |
|---|---|
| `block_idx` | Memory block index (0 - 9) |
| `enabled` | `1` = enabled, `0` = disabled |
| `data_type` | 0 = Coil, 1 = Discrete Input, 2 = Input Register, 3 = Holding Register |
| `writable` | `1` = allow master writes, `0` = read-only |
| `start_address` | First Modbus address (0 - 65535) |
| `count` | Number of registers/coils (1 - 10) |
| `encoding` | 0 = ABCD, 1 = BADC, 2 = CDAB, 3 = DCBA |
| `tag0` - `tag9` | Tag handle for each register offset (0 - 127, or 255 = not mapped) |
