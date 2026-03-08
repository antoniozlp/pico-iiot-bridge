# Network Settings

The Network Settings page allows you to configure the Ethernet network parameters of the Pico I-IoT Bridge.

![Network Settings](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/Network.png)

## Parameters

| Parameter | Description | Format / Valid Values |
|---|---|---|
| **MAC Address** | Hardware address for the Ethernet interface | `XX:XX:XX:XX:XX:XX` (hex, colon-separated) |
| **IP Address** | Device IP address on the network | Dotted decimal (e.g., `192.168.1.100`) |
| **Subnet Mask** | Network subnet mask | Dotted decimal (e.g., `255.255.255.0`) |
| **Gateway** | Default gateway IP address | Dotted decimal (e.g., `192.168.1.1`) |
| **DNS Server** | DNS server IP address | Dotted decimal (e.g., `8.8.8.8`) |
| **DHCP Mode** | IP address assignment method | `Static` or `DHCP` |

## DHCP Mode

When **DHCP** is selected, the device will automatically obtain an IP address, subnet mask, gateway, and DNS server from your network's DHCP server. The fields on this page will show the currently assigned values.

When **Static** is selected, you must manually configure all network parameters. The device will use the exact IP address and settings you provide.

> **Default**: The device ships with DHCP enabled. If no DHCP server is available, it falls back to a default static IP configuration.

## Saving Network Settings

1. Modify the desired parameters
2. Click **"Save Network"**
3. A confirmation dialog will appear on success
4. Click **"Reboot to Apply"** in the sidebar to activate the new network configuration

> **Warning**: If you change the IP address or DHCP mode, you may lose connectivity to the web interface after reboot. Make sure you know the new IP address before rebooting. When switching from DHCP to Static, note the current IP if you want to keep using it.

## CGI Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_network.cgi` | GET | Returns current network configuration as JSON |
| `set_network.cgi` | POST | Updates network configuration |

### GET Response Example

```json
{
  "mac": "00:08:DC:12:34:56",
  "ip": "192.168.68.137",
  "sn": "255.255.255.0",
  "gw": "192.168.68.1",
  "dns": "8.8.4.4",
  "dhcp": 2
}
```

### POST Parameters

| Parameter | Description |
|---|---|
| `mac` | MAC address string |
| `ip` | IP address string |
| `sn` | Subnet mask string |
| `gw` | Gateway string |
| `dns` | DNS server string |
| `dhcp` | `1` = Static, `2` = DHCP |
