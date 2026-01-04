#ifndef PICO_FLASH_STORAGE_H
#define PICO_FLASH_STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Initialize the flash storage system.
 *        Should be called before any read/write operations.
 */
void flash_storage_init(void);

/**
 * @brief Write data to flash safely in an SMP FreeRTOS environment.
 * 
 * This function handles:
 * 1. Pausing Core 1
 * 2. Disabling interrupts
 * 3. Erasing the target sector
 * 4. Programming the data
 * 
 * @param offset Offset from the start of flash (must be sector aligned for now)
 * @param data Pointer to the data to write
 * @param size Size of data (will be rounded up to page size)
 * @return true on success, false on failure (e.g. alignment issues)
 */
bool flash_storage_write(uint32_t offset, const uint8_t *data, size_t size);

/**
 * @brief Read data from flash.
 * 
 * @param offset Offset from the start of flash
 * @param data Buffer to store read data
 * @param size Number of bytes to read
 */
void flash_storage_read(uint32_t offset, uint8_t *data, size_t size);

#endif /* PICO_FLASH_STORAGE_H */

