# Serial to TCP Settings

The Serial to TCP page configures the transparent serial-to-Ethernet bridge mode. When enabled, data received on a serial port is forwarded over a TCP connection (and vice versa), allowing remote access to serial devices over the network.

![Serial to TCP Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/SerialToTCP.png)

## Parameters

| Parameter | Description | Valid Values |
|---|---|---|
| **Enable Serial to TCP** | Enable or disable the bridge function | Enabled / Disabled |
| **Serial Port** | Which UART to bridge | Serial 0, Serial 1 |
| **Operation Mode** | TCP connection role | Server (Listen) / Client (Connect) |
| **Local Port** | TCP port for the connection | 1024 - 65535 |
| **Idle Timeout (s)** | Close connection after this many seconds of inactivity | 1 - 3600 |
| **TCP Alive Check (s)** | Keepalive interval to detect dead connections | 1 - 600 |
| **Max Connections (1-4)** | Maximum simultaneous TCP connections (Server mode) | 1 - 4 |

### Client Mode Additional Parameters

These fields appear only when **Operation Mode** is set to **Client (Connect)**:

| Parameter | Description | Valid Values |
|---|---|---|
| **Remote IP Address** | IP of the remote TCP server to connect to | Dotted decimal (e.g., `192.168.1.100`) |
| **Remote Port** | TCP port of the remote server | 1024 - 65535 |

## Operation Modes

### Server Mode (Listen)

In **Server** mode, the device listens on the configured **Local Port** for incoming TCP connections. When a client connects:

1. Data received from the TCP client is forwarded to the serial port
2. Data received from the serial port is sent back to the TCP client
3. Multiple clients can connect simultaneously (up to **Max Connections**)
4. Idle connections are closed after the **Idle Timeout**

This is useful when you want to connect to a serial device from a remote application (e.g., a PC running a terminal emulator or SCADA software).

### Client Mode (Connect)

In **Client** mode, the device actively connects to a remote TCP server at the configured **Remote IP Address** and **Remote Port**:

1. The device initiates the TCP connection on startup
2. Data flows bidirectionally between the serial port and the remote server
3. If the connection drops, the device will attempt to reconnect

This is useful when the serial device needs to report to a central server.

## Example Use Cases

### Remote Serial Console
- **Mode**: Server
- **Serial Port**: Serial 1
- **Local Port**: 5000
- Connect from your PC using a TCP terminal (e.g., PuTTY, Tera Term) to `<device-ip>:5000`

### Serial Device Reporting to Central Server
- **Mode**: Client
- **Serial Port**: Serial 1
- **Remote IP**: IP of your data collection server
- **Remote Port**: Port your server is listening on

## Saving Settings

1. Configure the parameters
2. Click **"Save Serial to TCP"**
3. Click **"Reboot to Apply"** in the sidebar to activate the bridge

> **Note**: Make sure the serial port settings (baud rate, data bits, etc.) are configured correctly on the [Serial Settings](Web-Interface:-Serial-Settings) page to match your connected serial device.

## CGI Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_s2tcp.cgi` | GET | Returns current Serial-to-TCP configuration as JSON |
| `set_s2tcp.cgi` | POST | Updates Serial-to-TCP configuration |

### GET Response Example

```json
{
  "enable": 0,
  "serial": 1,
  "mode": 0,
  "lport": 5000,
  "timeout": 30,
  "keepalive": 5,
  "maxconn": 1,
  "remoteip": "0.0.0.0",
  "remoteport": 0
}
```

### POST Parameters

| Parameter | Description |
|---|---|
| `enable` | `1` = enabled, `0` = disabled |
| `serial` | Serial port index: `0` or `1` |
| `mode` | `0` = Server, `1` = Client |
| `lport` | Local TCP port (1024 - 65535) |
| `timeout` | Idle timeout in seconds (1 - 3600) |
| `keepalive` | Keepalive interval in seconds (1 - 600) |
| `maxconn` | Max connections (1 - 4) |
| `remoteip` | Remote IP address (Client mode) |
| `remoteport` | Remote TCP port (Client mode) |
