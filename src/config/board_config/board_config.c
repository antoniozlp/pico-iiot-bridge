#include "board_config.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

/**
 * @brief Initialize board-specific hardware configuration
 * 
 * This function initializes all GPIO pins, peripherals, and hardware
 * configurations specific to the board. Call this function early in
 * your main() before using any peripherals.
 */
void board_init_gpio(void)
{

    // =============================================================================
    // LED Configuration
    // =============================================================================
    gpio_init(BOARD_LED_PIN);
    gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
    gpio_put(BOARD_LED_PIN, 0);  // LED off initially

    // =============================================================================
    // Digital Inputs Configuration
    // =============================================================================
    gpio_init(BOARD_DIN0_PIN);
    gpio_set_dir(BOARD_DIN0_PIN, GPIO_IN);
    gpio_pull_up(BOARD_DIN0_PIN);  // Enable pull-up resistor
    
    gpio_init(BOARD_DIN1_PIN);
    gpio_set_dir(BOARD_DIN1_PIN, GPIO_IN);
    gpio_pull_up(BOARD_DIN1_PIN);  // Enable pull-up resistor

    // =============================================================================
    // Digital Outputs Configuration
    // =============================================================================
    gpio_init(BOARD_DOUT0_PIN);
    gpio_set_dir(BOARD_DOUT0_PIN, GPIO_OUT);
    gpio_put(BOARD_DOUT0_PIN, 0);  // Output low initially
    
    gpio_init(BOARD_DOUT1_PIN);
    gpio_set_dir(BOARD_DOUT1_PIN, GPIO_OUT);
    gpio_put(BOARD_DOUT1_PIN, 0);  // Output low initially

    // Note: SPI, UART, and I2C peripherals are initialized by their respective
    // drivers when needed. Pin functions are set automatically by the SDK.
}

/**
 * @brief Initialize UART with board-specific pin configuration
 * 
 * @param uart Pointer to UART instance (uart0 or uart1)
 * @param serial_config Pointer to serial configuration structure
 * @return true if initialization successful, false otherwise
 */
bool board_init_uart(uart_inst_t *uart, serial_config_t *serial_config)
{
    if (uart == NULL || serial_config == NULL)
    {
        return false;
    }

    // Initialize UART with the specified baud rate
    uint32_t actual_baud = uart_init(uart, serial_config->baud);
    if (actual_baud == 0)
    {
        return false;
    }

    // Configure UART pins based on which UART instance
    if (uart == uart0)
    {
        // UART0 - Debug/Console
        gpio_set_function(BOARD_UART0_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(BOARD_UART0_RX_PIN, GPIO_FUNC_UART);
        // UART0 does not have hardware flow control pins on this board
    }
    else if (uart == uart1)
    {
        // UART1 - Serial Bridge
        gpio_set_function(BOARD_UART1_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(BOARD_UART1_RX_PIN, GPIO_FUNC_UART);
        
        // Configure hardware flow control if enabled
        if (serial_config->flow_control_cts || serial_config->flow_control_rts)
        {
            uart_set_hw_flow(uart, serial_config->flow_control_cts, serial_config->flow_control_rts);
            
            if (serial_config->flow_control_cts)
            {
                gpio_set_function(BOARD_UART1_CTS_PIN, GPIO_FUNC_UART);
            }
            if (serial_config->flow_control_rts)
            {
                gpio_set_function(BOARD_UART1_RTS_PIN, GPIO_FUNC_UART);
            }
        }
    }
    else
    {
        // Unknown UART instance
        return false;
    }

    // Set data format (databits, stopbits, parity)
    uart_set_format(uart, serial_config->databits, serial_config->stopbits, serial_config->parity);

    // Enable UART FIFO
    uart_set_fifo_enabled(uart, true);

    return true;
}

