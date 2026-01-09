#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include "pico/stdlib.h"
#include "system_config.h"

// =============================================================================
// LED Configuration
// =============================================================================
#define BOARD_LED_PIN               PICO_DEFAULT_LED_PIN

// =============================================================================
// UART Configuration
// =============================================================================
// UART0 - Debug/Console (default stdio)
#define BOARD_UART0_ID              uart0
#define BOARD_UART0_TX_PIN          0
#define BOARD_UART0_RX_PIN          1
// UART0 does not have flow control pins

// UART1 - Serial Bridge
#define BOARD_UART1_ID              uart1
#define BOARD_UART1_TX_PIN          4
#define BOARD_UART1_RX_PIN          5
#define BOARD_UART1_CTS_PIN         6
#define BOARD_UART1_RTS_PIN         7


// =============================================================================
// SPI Configuration (for WIZnet Ethernet chip)
// =============================================================================
// Currently defined in lib/WIZnet/port/ioLibrary_Driver/inc/wizchip_spi.h

// =============================================================================
// I2C Configuration (optional, for future expansion)
// =============================================================================
#define BOARD_I2C_PORT              i2c1
#define BOARD_I2C_SDA_PIN           2
#define BOARD_I2C_SCL_PIN           3

// =============================================================================
// GPIO Configuration (general purpose inputs/outputs)
// =============================================================================
// Digital Inputs
#define BOARD_DIN0_PIN              8
#define BOARD_DIN1_PIN              9

// Digital Outputs
#define BOARD_DOUT0_PIN             10
#define BOARD_DOUT1_PIN             11

// Analog Inputs (ADC)
#define BOARD_AIN0_PIN              26  // ADC0
#define BOARD_AIN1_PIN              27  // ADC1
#define BOARD_AIN2_PIN              28  // ADC2


// =============================================================================
// Board Initialization Functions
// =============================================================================
void board_init_gpio(void);

bool board_init_uart(uart_inst_t *uart, serial_config_t *serial_config);
#endif // _BOARD_CONFIG_H_
