# CLI Reference

The Pico I-IoT Bridge includes a Command Line Interface (CLI) accessible through the serial console (UART0). It provides advanced configuration, diagnostics, and device management directly from a terminal application.

## Connecting to the CLI

1. Connect the device to your computer via USB or a serial adapter on **UART0** (Serial 0)
2. Open a terminal application (PuTTY, Tera Term, minicom, screen, etc.)
3. Use the default serial settings:
   - **Baud Rate**: 115200
   - **Data Bits**: 8
   - **Parity**: None
   - **Stop Bits**: 1
   - **Flow Control**: None
4. Press Enter to see the `>` prompt

> **Tip**: The CLI coexists with the logger output. Log messages may appear between your typed characters, but the prompt and input are automatically redrawn after each log line so you never lose what you've typed.

## Command Overview

| Command | Description |
|---|---|
| [`help`](#help) | List all available commands |
| [`config`](#config) | Read, write, and save device configuration |
| [`task-stats`](#task-stats) | Show FreeRTOS task states and stack usage |
| [`uptime`](#uptime) | Display system uptime |
| [`reboot`](#reboot) | Reboot the device |

---

## help

Lists all registered CLI commands with their usage information.

```
> help

help:
 Lists all the registered commands

task-stats:
 Displays a table showing the state of each FreeRTOS task

reboot:
 Reboot the device

config - Configuration management
Usage:
  config read <serial0|serial1|network|s2tcp|device>
  config write serial0|serial1 <baud|databits|parity|stopbits|flowcts|flowrts> <value>
  config write network <ip|subnet|gateway|dns> <a.b.c.d>
  config write network mode <static|dhcp>
  config write s2tcp <enable|serial|mode|port|timeout|keepalive|maxconn|remoteip|remoteport> <value>
  config write device <deviceid|loglevel|timestamp|taskname|colors> <value>
  config save
Type 'config write' for detailed parameter ranges.

uptime - Show system uptime
```

You can also type `help <command>` for details about a specific command.

---

## config

The `config` command is the main configuration interface. It has three sub-commands: `read`, `write`, and `save`.

### config read

Displays the current configuration for a section.

**Syntax:**

```
config read <serial0|serial1|network|s2tcp|device>
```

#### Read Network Configuration

```
> config read network
Network Configuration:
  MAC         : 00:08:DC:12:34:56
  IP          : 192.168.68.137
  Subnet Mask : 255.255.255.0
  Gateway     : 192.168.68.1
  DNS         : 8.8.4.4
  DHCP        : DHCP
```

#### Read Serial Configuration

```
> config read serial0
Serial0 Configuration:
  Baudrate: 115200
  Databits: 8
  Parity: 0
  Stopbits: 1
  Flow Control CTS: No
  Flow Control RTS: No
```

```
> config read serial1
Serial1 Configuration:
  Baudrate: 9600
  Databits: 8
  Parity: 0
  Stopbits: 1
  Flow Control CTS: No
  Flow Control RTS: No
```

#### Read Serial-to-TCP Configuration

```
> config read s2tcp
Serial-to-TCP Configuration:
  Enabled         : No
  Serial Port     : UART1
  Mode            : Server
  Local Port      : 5000
  Timeout         : 30 seconds
  Keepalive       : 5 seconds
  Max Connections : 1
```

When mode is set to Client, the output also shows remote IP and port:

```
> config read s2tcp
Serial-to-TCP Configuration:
  Enabled         : Yes
  Serial Port     : UART1
  Mode            : Client
  Local Port      : 5000
  Timeout         : 30 seconds
  Keepalive       : 5 seconds
  Max Connections : 1
  Remote IP       : 192.168.1.100
  Remote Port     : 5000
```

#### Read Device Configuration

```
> config read device
Device Configuration:
  Device ID       : PicoBridge01
  Log Level       : INFO
  Timestamp       : Yes
  Task Name       : Yes
  Colors          : Yes
```

---

### config write

Modifies a single configuration parameter in RAM. Changes are **not** saved to flash until you run `config save`.

**Syntax:**

```
config write <section> <field> <value>
```

#### Serial Parameters

| Field | Valid Values | Example |
|---|---|---|
| `baud` | 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 | `config write serial1 baud 9600` |
| `databits` | 5 - 8 | `config write serial1 databits 8` |
| `parity` | `none`, `even`, `odd` | `config write serial1 parity even` |
| `stopbits` | 1, 2 | `config write serial1 stopbits 1` |
| `flowcts` | 0 (disabled), 1 (enabled) | `config write serial1 flowcts 0` |
| `flowrts` | 0 (disabled), 1 (enabled) | `config write serial1 flowrts 0` |

**Examples:**

```
> config write serial1 baud 9600
Serial baudrate set to 9600

> config write serial1 parity even
Serial parity set to EVEN

> config write serial0 baud 115200
Serial baudrate set to 115200
```

#### Network Parameters

| Field | Valid Values | Example |
|---|---|---|
| `ip` | Dotted decimal IP | `config write network ip 192.168.1.100` |
| `subnet` | Dotted decimal IP | `config write network subnet 255.255.255.0` |
| `gateway` | Dotted decimal IP | `config write network gateway 192.168.1.1` |
| `dns` | Dotted decimal IP | `config write network dns 8.8.8.8` |
| `mode` | `static`, `dhcp` | `config write network mode static` |

**Examples:**

```
> config write network ip 192.168.1.50
IP address set to 192.168.1.50

> config write network mode dhcp
Network mode set to DHCP

> config write network dns 8.8.8.8
DNS set to 8.8.8.8
```

#### Serial-to-TCP Parameters

| Field | Valid Values | Example |
|---|---|---|
| `enable` | 0 (disabled), 1 (enabled) | `config write s2tcp enable 1` |
| `serial` | 0, 1 | `config write s2tcp serial 1` |
| `mode` | `server`, `client` | `config write s2tcp mode server` |
| `port` | 1024 - 65535 | `config write s2tcp port 5000` |
| `timeout` | 1 - 3600 (seconds) | `config write s2tcp timeout 60` |
| `keepalive` | 1 - 600 (seconds) | `config write s2tcp keepalive 10` |
| `maxconn` | 1 - 4 | `config write s2tcp maxconn 2` |
| `remoteip` | Dotted decimal IP | `config write s2tcp remoteip 192.168.1.100` |
| `remoteport` | 1024 - 65535 | `config write s2tcp remoteport 5000` |

**Examples:**

```
> config write s2tcp enable 1
Serial-to-TCP mode enabled

> config write s2tcp mode server
Serial-to-TCP mode set to SERVER

> config write s2tcp port 5000
Serial-to-TCP port set to 5000
```

#### Device Parameters

| Field | Valid Values | Example |
|---|---|---|
| `deviceid` | String (max ~31 characters) | `config write device deviceid PicoBridge01` |
| `loglevel` | `error`, `warn`, `info`, `debug` | `config write device loglevel debug` |
| `timestamp` | 0 (disabled), 1 (enabled) | `config write device timestamp 1` |
| `taskname` | 0 (disabled), 1 (enabled) | `config write device taskname 1` |
| `colors` | 0 (disabled), 1 (enabled) | `config write device colors 0` |

**Examples:**

```
> config write device loglevel debug
Log level set to DEBUG

> config write device deviceid MyBridge
Device ID set to 'MyBridge'

> config write device colors 0
Colors disabled
```

---

### config save

Persists all configuration changes to flash memory. You must run this after `config write` commands to make changes survive a reboot.

```
> config save
Configuration saved to flash successfully.
```

> **Important**: `config write` only modifies the in-memory configuration. If you reboot without running `config save`, your changes will be lost. This is different from the web interface, which saves to flash automatically on each Save button click.

---

## task-stats

Displays a table showing the state of each FreeRTOS task, including its priority, remaining stack space, and task number.

```
> task-stats
Task          State  Priority  Stack	#
************************************************
CLI             R       2       316     5
IDLE            R       0       218     3
Logger          B       3       322     2
HTTP Server     B       2       424     6
Network         B       3       636     1
Tmr Svc         B       2       225     4
IDLE            R       0       236     7
```

**Task States:**

| State | Meaning |
|---|---|
| **R** | Running (or Ready to run) |
| **B** | Blocked (waiting for event, delay, or resource) |
| **S** | Suspended |
| **D** | Deleted |

The **Stack** column shows the minimum free stack space (in words) that the task has had since it started. A very low number may indicate the task is close to a stack overflow.

---

## uptime

Displays how long the device has been running since the last reboot.

```
> uptime
Uptime: 0 days, 02:15:43
```

---

## reboot

Reboots the device immediately using the hardware watchdog timer. There is no confirmation prompt.

```
> reboot
Rebooting...
```

The device will restart and you will need to reconnect your terminal.

---

## Common Workflows

### First-Time Setup via CLI

Configure the device from scratch using only the serial console:

```
> config write network ip 192.168.1.100
IP address set to 192.168.1.100
> config write network subnet 255.255.255.0
Subnet mask set to 255.255.255.0
> config write network gateway 192.168.1.1
Gateway set to 192.168.1.1
> config write network mode static
Network mode set to Static
> config write serial1 baud 9600
Serial baudrate set to 9600
> config write serial1 parity even
Serial parity set to EVEN
> config save
Configuration saved to flash successfully.
> reboot
Rebooting...
```

### Enable Modbus RTU Serial Port

Configure Serial 1 for a typical Modbus RTU connection:

```
> config write serial1 baud 19200
Serial baudrate set to 19200
> config write serial1 databits 8
Serial databits set to 8
> config write serial1 parity even
Serial parity set to EVEN
> config write serial1 stopbits 1
Serial stopbits set to 1
> config save
Configuration saved to flash successfully.
```

### Enable Debug Logging

Temporarily increase log verbosity for troubleshooting:

```
> config write device loglevel debug
Log level set to DEBUG
> config write device timestamp 1
Timestamp enabled
> config write device taskname 1
Task name enabled
```

> **Note**: Logger settings (loglevel, timestamp, taskname, colors) take effect immediately without needing `config save` or a reboot. However, if you want them to persist across reboots, run `config save`.

### Check System Health

```
> uptime
Uptime: 1 days, 04:32:10
> task-stats
Task          State  Priority  Stack	#
************************************************
CLI             R       2       316     5
...
> config read network
Network Configuration:
  MAC         : 00:08:DC:12:34:56
  IP          : 192.168.68.137
  ...
```

## CLI vs Web Interface

Both the CLI and the [Web Configuration Interface](Web-Interface) can configure the same settings. Here are the key differences:

| Feature | CLI | Web Interface |
|---|---|---|
| **Connection** | Serial (UART0) | Ethernet (HTTP, port 80) |
| **Save behavior** | Manual (`config save`) | Automatic on each Save button |
| **Reboot** | `reboot` command | "Reboot to Apply" button |
| **Tag management** | Not available | Create, view, delete tags |
| **Modbus requests** | Not available | Configure up to 10 requests |
| **Task diagnostics** | `task-stats` command | Not available |
| **Uptime** | `uptime` command | Not available |
| **Log level control** | `config write device loglevel` | Not available |
| **Offline access** | Always available (physical cable) | Requires working network |

The CLI is especially useful for:
- Initial setup before network is configured
- Debugging when network connectivity is lost
- Monitoring task health and stack usage
- Adjusting log verbosity on the fly
