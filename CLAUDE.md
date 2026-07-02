# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Pico I-IoT Bridge: an industrial IoT protocol gateway firmware for the Raspberry Pi RP2350 (Pico 2) with a WIZnet W6100 Ethernet chip. Built on FreeRTOS with dual-core SMP. It bridges serial/Modbus devices to Ethernet (TCP/Modbus TCP), and is configurable via a web UI and a serial CLI. Currently developed/tested on the W6100-EVB-Pico2 board.

## Build

This project is built via CMake/Ninja using the Raspberry Pi Pico VS Code Extension, which manages the SDK toolchain (Pico SDK 2.2.0, ARM GCC 13.3 Rel1, CMake, Ninja, picotool) under `~/.pico-sdk/`. There is no plain system-wide toolchain assumption — paths in `.vscode/settings.json` and `.vscode/tasks.json` point into `~/.pico-sdk/`.

```bash
# One-time: initialize submodules (FreeRTOS-Kernel; WIZnet and nanoMODBUS are also submodules)
git submodule update --init --recursive

# Configure + build (mirrors the VS Code "Compile Project" task)
~/.pico-sdk/cmake/v3.31.5/bin/cmake -S . -B build -G Ninja
~/.pico-sdk/ninja/v1.12.1/ninja -C build

# Flash over SWD (OpenOCD) — mirrors the VS Code "Flash" task
# Or use picotool load <path-to.uf2> -fx after mounting the Pico in BOOTSEL mode
```

Build outputs land in `build/` (`pico-iiot-bridge.elf`, `.uf2`, `.hex`, `.dis`, `.elf.map`). There is no separate lint step or test suite in this repo — verification is by building cleanly and, ideally, flashing/exercising the device.

Check flash/RAM budget after significant changes (RP2350 has 2MB flash / 520KB SRAM):

```bash
python3 scripts/calc_memory.py build/pico-iiot-bridge.elf
```

### Board/chip selection

`CMakeLists.txt` hardcodes `BOARD_NAME` (currently `W6100_EVB_PICO2`) which drives `WIZNET_CHIP` and related `-D` defines. Change this if targeting a different WIZnet eval board (W5100S/W5500/W6300, RP2040 vs RP2350 variants) — don't add new board logic elsewhere.

### RTOS Views support

`add_definitions(-DRTOS_VIEWS_SUPPORT)` in the root `CMakeLists.txt` enables FreeRTOS runtime-stats/debug support (see `src/rtos_views_support/`). Comment it out for production builds where that overhead isn't wanted.

## Architecture

### Module layout

Each feature lives under `src/<module>/` with its own `CMakeLists.txt` producing a static/interface library; `add_subdirectory` is wired up centrally in the root `CMakeLists.txt`, and `src/main.c` links every module and calls its `*_init()` function in a fixed order. When adding a new module, follow this same pattern: own subdirectory, own CMakeLists, one `<module>_task_init(void) -> bool` entry point called from `main.c`.

Initialization order in `main.c` matters and encodes real dependencies: logger → flash storage → config (load from flash) → tag database (init, then load persisted tags) → network → HTTP server → CLI → serial-to-TCP → Modbus RTU master/slave → Modbus TCP client/server. Each `*_init()` returns `bool`; any failure logs and aborts boot (`return 1`) rather than degrading gracefully.

### Tag Database — the central data bus

`src/tag-database/` (`tag_database.h/.c`) is the hub all protocol tasks read/write through instead of talking to each other directly. Producers (e.g. Modbus clients) write named tags; consumers (e.g. Modbus servers, HTTP server) read them. Key properties:
- Values are a fixed-size union (`tag_value_t`, max 4 bytes) with a `tag_quality_t` (GOOD/BAD/UNCERTAIN) and timestamp.
- Writes are non-blocking (queued to a dedicated FreeRTOS task); reads are mutex-protected and synchronous.
- Tags are resolved by name to a `tag_handle_t` once at task init, then accessed by handle.
- Tag *definitions* (name + type) can be persisted to flash independently of tag *values* (`tag_db_create_persistent`, `tag_db_save_to_flash`, `tag_db_load_from_flash`); capacity differs between runtime (`TAG_DATABASE_MAX_TAGS` = 128) and flash-persisted (`TAG_DB_MAX_PERSISTENT_TAGS` = 64) tags — see `system_config.h`.

### Modbus stack — transport-agnostic core + 4 task variants

`src/modbus/` implements Modbus RTU client, RTU server, TCP client, and TCP server as four separate FreeRTOS tasks (`modbus_rtu_master.c`, `modbus_rtu_slave.c`, `modbus_tcp_client.c`, `modbus_tcp_server.c`), all built on the vendored `nanoMODBUS` library (`lib/nanoMODBUS`, a git submodule). Shared logic is factored out so it doesn't need to be duplicated per transport:
- `modbus_request.h` — shared config/result structs (`modbus_request_config_t`, `modbus_request_result_t`) used identically by RTU and TCP; includes the 32-bit word/byte order handling (`modbus_register_encoding_t`: ABCD/BADC/CDAB/DCBA) needed for cross-vendor register encoding.
- `modbus_request_processor.c/.h` — executes one request against any `nmbs_t` instance (RTU or TCP) — this is what makes the request logic transport-agnostic.
- `modbus_tag_mapping.c/.h` — maps Modbus registers/coils to/from tag database entries (`modbus_map_to_tags` / `modbus_map_from_tags`), including multi-register (INT32/FLOAT) reassembly.

When touching Modbus behavior, check whether the change belongs in the shared processor/mapping layer (affects all 4 tasks) versus one transport-specific task file.

### Configuration system

`src/config/` splits into `board_config` (static hardware/pin definitions) and `system_config` (runtime-configurable, flash-persisted settings: network, serial0/1, serial-to-TCP, Modbus RTU/TCP client & server, tag database, device settings). `system_config.h` defines the on-flash struct layout and a `CONFIG_VERSION_MAJOR/MINOR/PATCH` — bump the appropriate version field when changing the persisted struct layout, since `FLASH_TARGET_OFFSET` (last 64KB flash block) stores a single versioned blob (`CONFIG_BUFFER_SIZE`, page-aligned). Config is exposed to the rest of the app purely through `config_get_*`/`config_set_*` accessor pairs — never read the flash-backed struct directly from other modules.

### Flash storage (dual-core coordination)

`src/pico-flash-storage/` provides the only path to raw flash writes (`flash_storage_write`/`flash_storage_read`). Because flash erase/program must halt Core 1 while Core 0 writes (RP2350 SMP hazard), this module runs a coordinator task on Core 0 and a guard task on Core 1 pinned via `vTaskCoreAffinitySet`. Do not bypass this module to write flash directly from another task.

### HTTP server

`src/http-server/` serves a web UI whose HTML/CSS/JS are compiled in as C string headers under `html/` (`layout.h`, `css.h`, `js.h`, plus one header per page: `network.h`, `serial.h`, `tcp.h`, `modbus.h`, `modbus_server.h`, `modbus_tcp.h`, `tags.h`). Config is fetched by the page via one `get_<section>.cgi` endpoint per config section (`http_utils.c`), not a single monolithic endpoint — this was a deliberate split (see below) to keep each JSON response well under the ~2KB HTTP buffer (`g_http_send_buf`/`g_http_recv_buf`). When adding a new configurable section, add its own `get_<section>.cgi`/`set_<section>.cgi` pair rather than growing an existing endpoint's payload.

### CLI

`src/cli-task/` integrates FreeRTOS-Plus-CLI (`lib/FreeRTOS-Plus-CLI`) over UART0. Commands: `help`, `config read/write/save`, `task-stats`, `uptime`, `reboot`. Full syntax and examples are documented in `docs/wiki/CLI-Reference.md`. Note `config write` is in-RAM only — `config save` is required to persist to flash (unlike the web UI, which saves on every button click).

### Networking

`src/network/network_task.c` owns the WIZnet chip lifecycle (SPI init, DHCP or static IP, PHY link monitoring) and broadcasts status changes to other tasks via FreeRTOS task notifications (bitmask: `NETWORK_NOTIFY_LINK_UP/DOWN/READY/NOT_READY/IP_CHANGED`) rather than a shared status struct — other tasks call `network_task_register_notification()` to subscribe instead of polling.

### Logging

`src/logger/` provides thread-safe, queue-based logging (`LOG_ERROR/WARN/INFO/DEBUG`). Always use these macros instead of `printf` directly from tasks — concurrent `printf` from multiple FreeRTOS tasks is not safe on this stdio setup, and the CLI depends on log lines being redrawn correctly around the live prompt.

### Vendored dependencies (`lib/`)

- `lib/FreeRTOS-Kernel` — git submodule, RTOS core.
- `lib/WIZnet` — WIZnet `ioLibrary_Driver` (submodule) + `port/` glue for the W6100/W5x00 family.
- `lib/nanoMODBUS` — git submodule, Modbus RTU/TCP protocol library wrapped by `src/modbus/`.
- `lib/FreeRTOS-Plus-CLI` — vendored (not a submodule), used by `src/cli-task/`.

Don't modify vendored/submodule code in place; changes belong in the wrapping `src/` module.

## Coding standards (from `.cursor/rules/`)

These apply project-wide (see `.cursor/rules/*.mdc` for full examples):

- **Naming**: files/functions/locals `snake_case` with module prefix (`network_task_init`); FreeRTOS task functions `vNameTask`; globals `g_`, file-statics `s_` prefixed; types `snake_case_t`; enum values and macros `UPPER_CASE`.
- **Return values**: init/config/registration functions return `bool` (never bare `void` where failure is possible); FreeRTOS task functions are always `void`; use a dedicated error-code enum only when multiple distinct failure modes must be distinguished (e.g. `flash_result_t`).
- **Error handling**: check every FreeRTOS/SDK return value; clean up already-created resources (mutex/queue/task) on a later init step's failure, in reverse order.
- **Concurrency**: protect shared state with a mutex + timeout (never `portMAX_DELAY` on a mutex take); use task notifications or queues for cross-task signaling rather than polling shared flags; document any RP2350 dual-core affinity decisions.
- **Includes**: order is own header → C stdlib → FreeRTOS → Pico SDK/hardware → third-party (WIZnet/nanoMODBUS) → project headers; include what you use, don't rely on transitive includes; header guards are `_FILENAME_H_`.
- **General embedded safety**: no dynamic allocation in ISRs; named constants instead of magic numbers; `sizeof()` for buffer sizes; `volatile` for ISR-shared variables; every FreeRTOS task loop must yield (`vTaskDelay`), never spin unbounded.
- Existing code may not fully match these standards yet — apply them to new code and nearby edits, don't do unrelated mass rewrites.

## Documentation

- End-user/configuration docs live in `docs/wiki/` (mirrors the GitHub Wiki: Web Interface pages, `CLI-Reference.md`) — update these alongside behavior changes to CLI commands or web UI config fields.
- `README.md` has the feature list and hardware overview; keep "Current Capabilities" / "Planned Features" / "Development Roadmap" in sync when shipping a previously-planned feature (e.g. Modbus TCP was recently moved from planned to current).
