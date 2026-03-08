# Tag Database

The Tag Database page provides a centralized interface for managing the named variables (tags) that are shared between different protocols and subsystems in the Pico I-IoT Bridge.

Tags act as the glue between protocols: for example, a Modbus RTU request can write a register value into a tag, and a future MQTT client could read that same tag and publish it to a broker.

![Tag Database](https://raw.githubusercontent.com/antoniozlp/pico-iiot-bridge/main/docs/wiki/images/TagDatabase.png)

## Tag Management

### Creating a Tag

1. Click the **"Create New Tag"** button
2. Fill in the tag parameters:
   - **Tag Name**: A descriptive name up to 15 characters (e.g., `TEMP_S1`, `PRESURE_S1`, `COUNT_01`)
   - **Data Type**: The data type for this tag's value
3. Click **"Create Tag"**

The tag is immediately created and persisted to flash memory.

### Supported Data Types

| Type | Description | Value Range |
|---|---|---|
| **BOOL** | Boolean (true/false) | 0 or 1 |
| **UINT8** | Unsigned 8-bit integer | 0 - 255 |
| **UINT16** | Unsigned 16-bit integer | 0 - 65,535 |
| **UINT32** | Unsigned 32-bit integer | 0 - 4,294,967,295 |
| **INT16** | Signed 16-bit integer | -32,768 to 32,767 |
| **INT32** | Signed 32-bit integer | -2,147,483,648 to 2,147,483,647 |
| **FLOAT** | 32-bit floating point | IEEE 754 single precision |

### Deleting a Tag

Click the red **"Delete"** button in the Actions column for the tag you want to remove. A confirmation dialog will appear before deletion. The tag is removed from both the runtime database and flash memory.

> **Warning**: Deleting a tag that is mapped to a Modbus request will cause that mapping to become invalid. Update the [Modbus RTU](Web-Interface:-Modbus-RTU) request mappings accordingly.

## Active Tags Table

The table displays all tags currently configured in the system, with the following columns:

| Column | Description |
|---|---|
| **Handle** | Internal numeric identifier (assigned automatically) |
| **Tag Name** | The name you assigned when creating the tag |
| **Type** | Data type (BOOL, UINT8, UINT16, UINT32, INT16, INT32, FLOAT) |
| **Value** | Current value of the tag |
| **Quality** | Data quality indicator (GOOD, BAD, UNCERTAIN) |
| **Updated** | Time since the last value update (e.g., "38m ago") |
| **Actions** | Delete button |

### Quality Indicators

| Quality | Meaning |
|---|---|
| **GOOD** | Value has been recently updated and is considered valid |
| **BAD** | Communication error or data source is unavailable |
| **UNCERTAIN** | Value has not been updated yet or the source status is unknown |

> **Note**: Newly created tags start with quality **UNCERTAIN** and a value of 0 until a data source (e.g., Modbus client) writes to them.

### Auto-Refresh

The tag table refreshes automatically every 2 seconds to show updated values. You can also click the **"Refresh"** button to manually refresh at any time.

The tag count is displayed next to the Refresh button. If the total tag data exceeds the HTTP response buffer, a truncation warning will appear indicating how many tags could not be displayed.

## Naming Conventions

While tag names are free-form (up to 15 characters), a consistent naming scheme helps keep things organized. Some suggestions:

| Pattern | Example | Use Case |
|---|---|---|
| `SENSOR_NN` | `TEMP_S1`, `PRESURE_S1` | Sensor readings |
| `COUNT_NN` | `COUNT_01`, `COUNT_02` | Counter values |
| `CURRENT_MN` | `CURRENT_M1`, `CURRENT_M2` | Motor current measurements |
| `STATUS_XX` | `STATUS_PMP1` | Device status flags |
| `CMD_XX` | `CMD_VALVE1` | Command/control outputs |

## CGI Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `get_tags.cgi` | GET | Returns all tags as a JSON array |
| `create_tag.cgi` | POST | Creates a new tag |
| `delete_tag.cgi` | POST | Deletes a tag by name |

### GET Response Example (get_tags.cgi)

```json
{
  "tags": [
    {
      "handle": 0,
      "name": "TEMP_S1",
      "type": 6,
      "value": 0.00,
      "quality": 2,
      "age": 2280
    },
    {
      "handle": 1,
      "name": "PRESURE_S1",
      "type": 6,
      "value": 0.00,
      "quality": 2,
      "age": 2280
    }
  ]
}
```

Type values: 0=BOOL, 1=UINT8, 2=UINT16, 3=UINT32, 4=INT16, 5=INT32, 6=FLOAT

Quality values: 0=GOOD, 1=BAD, 2=UNCERTAIN

### POST Parameters (create_tag.cgi)

| Parameter | Description |
|---|---|
| `tag_name` | Tag name (max 15 characters) |
| `data_type` | Data type index (0-6, see type values above) |

### POST Parameters (delete_tag.cgi)

| Parameter | Description |
|---|---|
| `tag_name` | Name of the tag to delete |
