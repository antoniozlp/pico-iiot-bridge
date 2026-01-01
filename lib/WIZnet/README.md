# WIZnet Pico Port

This port directory contains the MCU-dependent code for WIZnet Ethernet chips on Raspberry Pi Pico (RP2040/RP2350) platforms.

## Origin

This code was extracted from the [WIZnet-PICO-C](https://github.com/WIZnet-ioNIC/WIZnet-PICO-C) repository, which provides comprehensive Ethernet examples for RP2040 and RP2350 microcontrollers.

**Source Repository:** https://github.com/WIZnet-ioNIC/WIZnet-PICO-C

## Purpose

This port provides the hardware abstraction layer (HAL) for WIZnet Ethernet chips, including:

- **SPI Communication**: Low-level SPI read/write functions for communicating with WIZnet chips
- **QSPI Support**: PIO-based QSPI implementation for high-performance boards (W55RP20, W6300)
- **GPIO Interrupt Handling**: Interrupt callbacks for Ethernet chip events
- **Timer Functions**: 1ms timer callbacks and delay functions

## Directory Structure

```
wiznet-pico-port/
├── ioLibrary_Driver/
│   ├── inc/
│   │   ├── wizchip_spi.h           # SPI interface definitions
│   │   ├── wizchip_gpio_irq.h      # GPIO interrupt handling
│   │   └── wizchip_qspi_pio.h      # QSPI PIO interface (W55RP20/W6300)
│   └── src/
│       ├── wizchip_spi.c           # SPI implementation
│       ├── wizchip_gpio_irq.c      # Interrupt implementation
│       ├── wizchip_qspi_pio.c      # QSPI implementation
│       └── wizchip_qspi_pio.pio    # PIO state machine for QSPI
├── timer/
│   ├── timer.h                     # Timer interface
│   └── timer.c                     # Timer implementation
├── board_list.h                    # Board-specific definitions
├── port_common.h                   # Common port includes
└── CMakeLists.txt                  # Build configuration
```

## Supported Boards

- Raspberry Pi Pico (RP2040)
- Raspberry Pi Pico 2 (RP2350)
- W55RP20-EVB-PICO
- W6300-EVB-PICO
- W6300-EVB-PICO2
- W5500-EVB-PICO
- W5100S-EVB-PICO

## Key Features

### SPI Interface
- Hardware SPI support with DMA
- Configurable SPI pins and settings
- Critical section locking for thread safety
- Burst read/write operations

### QSPI/PIO Support
- PIO-based QSPI implementation for W55RP20 and W6300
- Single/Dual/Quad SPI modes
- High-performance data transfer

### Timer Services
- 1ms repeating timer for protocol stacks
- Millisecond delay functions
- Low-power sleep support

### GPIO Interrupts
- Configurable interrupt callbacks per socket
- Event-driven packet reception

## Configuration

All board-specific **SPI pin settings** can be configured in `ioLibrary_Driver/inc/wizchip_spi.h`.

PIO-related configurations for W55RP20 and W6300 boards should be made in:
- `wizchip_qspi_pio.c`
- `wizchip_qspi_pio.h`
- `wizchip_qspi_pio.pio`

## License

Copyright (c) 2021 WIZnet Co., Ltd

SPDX-License-Identifier: BSD-3-Clause

## Credits

Original code from [WIZnet-ioNIC/WIZnet-PICO-C](https://github.com/WIZnet-ioNIC/WIZnet-PICO-C) by WIZnet Co., Ltd.
