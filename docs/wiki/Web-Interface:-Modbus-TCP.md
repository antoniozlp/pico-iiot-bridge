# Modbus TCP Settings

The Modbus TCP section of the web interface covers two independent features:

- **Modbus TCP Client** — connects to a remote Modbus TCP server over Ethernet, polls data according to configured requests, and writes values into the Tag Database
- **Modbus TCP Server** — listens on a TCP port and exposes Tag Database tags as Modbus registers/coils so an external Modbus TCP client (PLC, SCADA, HMI, etc.) can read and write them

Both features use the device’s Ethernet interface. Unlike [Modbus RTU](Web-Interface:-Modbus-RTU), there is no serial port selection—communication is entirely over TCP/IP.

---

## Modbus TCP Client

![Modbus TCP Client Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/ModbusTCPClient.png)

### Client Settings

The top section configures how the device connects to the remote Modbus TCP server:

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable Modbus TCP Client** | Enable or disable the Modbus TCP client task | Enabled / Disabled |
| **Remote Server IP** | IPv4 address of the Modbus TCP server to poll | Dotted decimal (e.g. `192.168.1.100`) |
| **Remote Port** | TCP port on the remote server | 1 - 65535 (Modbus TCP default is **502**) |

Click **"Save Client Settings"** to save these parameters.

> **Note**: When the client is enabled, it connects to **Remote Server IP**:**Remote Port**, keeps the TCP session open while the network is up, and repeatedly runs all enabled requests on that connection (with automatic reconnect if the connection drops). Ensure the remote device accepts Modbus TCP on that address and port, and that routing and firewall rules allow the connection.

### Modbus TCP Requests

Below the client settings, you can configure up to **10 independent Modbus requests** (Request 0 through Request 9). Each request defines a read or write operation against the remote server. Select a request using the numbered tab buttons.

#### Request Parameters

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable** | Enable or disable this request | Enabled / Disabled |
| **Unit ID (1-247)** | Modbus unit identifier in the TCP ADU (often called slave address) | 1 - 247 |
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

When reading or writing 32-bit values (e.g., FLOAT, UINT32) that span two consecutive 16-bit registers, the byte order matters:

| Encoding | Description | Byte Order |
|---|---|---|
| **ABCD** | Big-endian (most common) | High word first, high byte first |
| **BADC** | Word swap | High word first, low byte first |
| **CDAB** | Byte swap | Low word first, high byte first |
| **DCBA** | Word + byte swap | Low word first, low byte first |

> **Tip**: If floating-point values look wrong, try a different encoding. ABCD is the most common; device manuals often specify the expected order.

### Tag Mapping (Client)

Each request includes a **Tag Mapping** section that links individual registers or coils to tags in the [Tag Database](Web-Interface:-Tag-Database).

- **Reg/Coil 0** through **Reg/Coil 9** correspond to the registers starting at **Start Address**
- Select a tag from the dropdown to map that register to it, or choose **"Not Mapped"** to skip
- Tags must be created first on the [Tag Database](Web-Interface:-Tag-Database) page before they appear here

Click **"Save Request N"** to save the configuration for that request index.

### Saving Client Settings

1. Configure **Client Settings** and click **"Save Client Settings"**
2. For each request tab, set parameters and tag mappings, then click **"Save Request N"**
3. Click **"Reboot to Apply"** in the sidebar so the Modbus TCP client runs with the new configuration

---

## Modbus TCP Server

The Modbus TCP Server makes the Pico I-IoT Bridge act as a **Modbus TCP slave** (server). External clients connect via Ethernet and read or write Tag Database variables using standard Modbus TCP.

![Modbus TCP Server Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/ModbusTCPServer.png)

### Server Settings

The top section configures the listening server:

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable Modbus TCP Server** | Enable or disable the Modbus TCP server task | Enabled / Disabled |
| **Listen Port** | TCP port on which the device accepts Modbus TCP connections | 1 - 65535 (default **502**) |
| **Server Unit ID (1-247)** | Unit ID this server uses in Modbus TCP ADUs | 1 - 247 |

Click **"Save Server Settings"** to save these parameters.

> **Port 502**: Many networks treat port 502 specially. If the web UI or another service already uses a port, choose a different listen port and configure your Modbus client to match.

### Server Memory Map (Memory Blocks)

Below the server settings, you configure up to **10 memory blocks** (Block 0 through Block 9). Each block defines a contiguous Modbus address range backed by tag mappings. An incoming client request must fall entirely within a single enabled block—requests that span two blocks or unmapped ranges receive a Modbus exception (typically `ILLEGAL_DATA_ADDRESS`).

Select a block using the numbered tab buttons.

#### Memory Block Parameters

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable** | Enable or disable this memory block | Enabled / Disabled |
| **Data Type** | The Modbus address space this block occupies | See table below |
| **Allow Client Writes** | Whether remote clients may write this block | No (read-only), Yes (read/write) |
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

> **Write access**: For Coil and Holding Register blocks, write function codes are only accepted if **Allow Client Writes** is **Yes (read/write)**. Otherwise the server may respond with `ILLEGAL_FUNCTION` for write attempts.

#### 32-bit Encoding

The same encoding options as the client apply. This controls how 32-bit tag values (FLOAT, UINT32, INT32) are packed into two consecutive 16-bit registers when clients read or write them.

### Tag Mapping (Server)

Each memory block includes **Tag Mapping** that links Modbus addresses to tags:

- **Reg/Coil 0** through **Reg/Coil 9** map to consecutive addresses starting from **Start Address**
- **"Not Mapped"** leaves that address without a tag; reads typically return `0`, and writes may be ignored (see firmware behavior for your version)

Tags must exist on the [Tag Database](Web-Interface:-Tag-Database) page before they appear in the dropdowns.

Click **"Save Memory Block N"** to save that block.

### Saving Server Settings

1. Configure **Server Settings** and click **"Save Server Settings"**
2. For each memory block tab, configure parameters and tag mappings, then click **"Save Memory Block N"**
3. Click **"Reboot to Apply"** in the sidebar so the Modbus TCP server uses the new configuration

---

## CGI Endpoints

### Client Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_modbus_tcp_client.cgi` | GET | Returns full Modbus TCP client configuration (settings + all requests) as JSON |
| `set_modbus_tcp_client.cgi` | POST | Updates client settings (enable, remote IP, port) |
| `set_modbus_tcp_datapoint.cgi` | POST | Updates a single request configuration |

#### GET Response Example (`get_modbus_tcp_client.cgi`)

```json
{
  "enable": 0,
  "remote_ip": "192.168.11.100",
  "remote_port": 502,
  "requests": [
    {
      "enabled": 0,
      "slave_address": 1,
      "data_type": 3,
      "operation": 0,
      "start_address": 0,
      "count": 1,
      "encoding": 0,
      "tag_handles": [255, 255, 255, 255, 255, 255, 255, 255, 255, 255]
    }
  ]
}
```

In `tag_handles`, `255` means **Not Mapped**. Other values are tag handle indices from the Tag Database.

#### POST Parameters (`set_modbus_tcp_client.cgi`)

| Parameter | Description |
|---|---|
| `enable` | `1` = enabled, `0` = disabled |
| `remote_ip` | Remote server IPv4 address (dotted string) |
| `remote_port` | TCP port (1 - 65535) |

#### POST Parameters (`set_modbus_tcp_datapoint.cgi`)

| Parameter | Description |
|---|---|
| `dp_idx` | Request index (0 - 9) |
| `enabled` | `1` = enabled, `0` = disabled |
| `slave_address` | Unit ID (1 - 247) |
| `data_type` | `0` = Coil, `1` = Discrete Input, `2` = Input Register, `3` = Holding Register |
| `operation` | `0` = Read, `1` = Write |
| `start_address` | Start address (0 - 65535) |
| `count` | Register/coil count (1 - 10) |
| `encoding` | `0` = ABCD, `1` = BADC, `2` = CDAB, `3` = DCBA |
| `tag0` - `tag9` | Tag handle for each register (0 - 127, or 255 = not mapped) |

---

### Server Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_modbus_tcp_server.cgi` | GET | Returns full Modbus TCP server configuration (settings + all memory blocks) as JSON |
| `set_modbus_tcp_server.cgi` | POST | Updates server settings (enable, listen port, server unit ID) |
| `set_modbus_tcp_server_memory_block.cgi` | POST | Updates a single memory block configuration |

#### GET Response Example (`get_modbus_tcp_server.cgi`)

```json
{
  "enable": 0,
  "port": 502,
  "server_address": 1,
  "memory_blocks": [
    {
      "enabled": 0,
      "data_type": 3,
      "writable": 0,
      "start_address": 0,
      "count": 1,
      "encoding": 0,
      "tag_handles": [255, 255, 255, 255, 255, 255, 255, 255, 255, 255]
    }
  ]
}
```

In `tag_handles`, `255` means **Not Mapped**.

#### POST Parameters (`set_modbus_tcp_server.cgi`)

| Parameter | Description |
|---|---|
| `enable` | `1` = enabled, `0` = disabled |
| `port` | Listen port (1 - 65535) |
| `server_address` | Server unit ID (1 - 247) |

#### POST Parameters (`set_modbus_tcp_server_memory_block.cgi`)

| Parameter | Description |
|---|---|
| `block_idx` | Memory block index (0 - 9) |
| `enabled` | `1` = enabled, `0` = disabled |
| `data_type` | `0` = Coil, `1` = Discrete Input, `2` = Input Register, `3` = Holding Register |
| `writable` | `1` = allow client writes, `0` = read-only |
| `start_address` | First Modbus address (0 - 65535) |
| `count` | Number of registers/coils (1 - 10) |
| `encoding` | `0` = ABCD, `1` = BADC, `2` = CDAB, `3` = DCBA |
| `tag0` - `tag9` | Tag handle for each register offset (0 - 127, or 255 = not mapped) |
