# Applying Changes

Most configuration changes in the Pico I-IoT Bridge are **saved to flash immediately** when you click a Save button, but they **only take effect after a device reboot**. This page explains the save-and-reboot workflow.

## How Configuration Persistence Works

1. **You click Save** on any configuration page
2. The new settings are sent to the device via an HTTP POST request
3. The device validates the parameters and writes them to **flash memory**
4. A success confirmation is displayed in the browser
5. The settings are now stored persistently but **not yet active**

## Rebooting the Device

To activate saved configuration changes, click the **"Reboot to Apply"** button located at the bottom of the sidebar menu. This button is visible on every page.

A confirmation dialog will ask: *"Reboot device now to apply configuration changes?"*

- Click **OK** to proceed with the reboot
- Click **Cancel** to stay on the current page

After confirming, the device performs a software reset via the hardware watchdog timer. The reboot takes a few seconds. You will need to refresh or re-navigate to the web interface once the device is back online.

> **Note**: If you changed the IP address or switched between DHCP and Static mode, the device may come up on a different IP address after reboot.

## What Requires a Reboot?

| Setting | Requires Reboot? |
|---|---|
| Network settings (IP, DHCP, etc.) | **Yes** |
| Serial port settings (baud, parity, etc.) | **Yes** |
| Serial to TCP configuration | **Yes** |
| Modbus RTU client settings | **Yes** |
| Modbus RTU request configuration | **Yes** |
| Tag creation / deletion | **No** (immediate effect) |

Tags are the exception: creating or deleting a tag takes effect immediately without requiring a reboot.

## Multiple Changes Before Reboot

You can make changes across multiple configuration pages before rebooting. All saved settings accumulate in flash memory and will all take effect together on the next reboot. This avoids unnecessary multiple reboots when configuring the device for the first time.

**Recommended first-time setup order:**

1. Configure **Network Settings** (set a static IP if desired)
2. Configure **Serial Settings** for the ports you plan to use
3. Configure **Serial to TCP** or **Modbus RTU** as needed
4. Create **Tags** in the Tag Database (if using Modbus with tag mapping)
5. Configure **Modbus RTU Requests** with tag mappings
6. Click **"Reboot to Apply"** once to activate everything

## Troubleshooting

### Lost connection after reboot
If you can't reach the web interface after reboot:
- Check if the IP address changed (especially after switching DHCP modes)
- Verify the Ethernet cable is connected
- Use the serial console (Serial 0) to check the device's current IP address
- If the device is in DHCP mode, check your router's DHCP client list

### Settings not taking effect
If settings don't seem to apply after reboot:
- Make sure you clicked the **Save** button on the relevant page (check for the success confirmation)
- Verify the reboot actually happened (the page will become unresponsive momentarily during reboot)
- Use the serial console to check for error messages during startup
