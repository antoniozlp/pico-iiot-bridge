/**
 * @file tag_database.h
 * @brief Centralized tag database for variable sharing across tasks
 * 
 * Provides a named variable database where producer tasks write values
 * and consumer tasks read values. Uses FreeRTOS queue-based communication
 * for non-blocking writes and mutex-protected reads.
 * 
 * Example usage:
 *   // Create tag at initialization
 *   tag_handle_t temp_handle = tag_db_create("TEMP_SENSOR_01", TAG_TYPE_FLOAT);
 *   
 *   // Producer writes value
 *   tag_value_t value;
 *   value.float_val = 25.3;
 *   tag_db_write(temp_handle, value, TAG_QUALITY_GOOD);
 *   
 *   // Consumer reads value
 *   tag_db_read(temp_handle, &value, NULL, NULL);
 *   float temperature = value.float_val;
 */

#ifndef _TAG_DATABASE_H_
#define _TAG_DATABASE_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// ============================================================================
// Configuration
// ============================================================================

#define TAG_NAME_MAX_LEN            16      // Tag name length (15 chars + null)
#define TAG_DATABASE_MAX_TAGS       128     // Maximum number of tags

// Queue configuration
#define TAG_DB_QUEUE_LENGTH         32      // Pending write messages
#define TAG_DB_TASK_PRIORITY        (tskIDLE_PRIORITY + 3)
#define TAG_DB_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 4)

// ============================================================================
// Data Types
// ============================================================================

/**
 * @brief Tag data types
 */
typedef enum {
    TAG_TYPE_BOOL = 0,
    TAG_TYPE_UINT8,
    TAG_TYPE_UINT16,
    TAG_TYPE_UINT32,
    TAG_TYPE_INT16,
    TAG_TYPE_INT32,
    TAG_TYPE_FLOAT,
    TAG_TYPE_COUNT
} tag_data_type_t;

/**
 * @brief Tag quality indicator (OPC UA / SCADA style)
 */
typedef enum {
    TAG_QUALITY_GOOD = 0,       // Valid, up-to-date value
    TAG_QUALITY_BAD,            // Sensor error, communication failure
    TAG_QUALITY_UNCERTAIN,      // Stale data, last known value
} tag_quality_t;

/**
 * @brief Tag value union (32-bit maximum)
 */
typedef union {
    bool bool_val;
    uint8_t u8_val;
    uint16_t u16_val;
    uint32_t u32_val;
    int16_t i16_val;
    int32_t i32_val;
    float float_val;
} tag_value_t;

/**
 * @brief Tag handle (opaque identifier for fast access)
 */
typedef uint8_t tag_handle_t;
#define TAG_HANDLE_INVALID 0xFF

/**
 * @brief Tag metadata
 */
typedef struct {
    char name[TAG_NAME_MAX_LEN];    // Human-readable name
    tag_data_type_t data_type;      // Value type
    tag_quality_t quality;          // Current quality status
    uint32_t timestamp_ms;          // Last update time
    tag_value_t value;              // Current value
} tag_metadata_t;

/**
 * @brief Queue message types
 */
typedef enum {
    TAG_MSG_WRITE = 0,              // Write value to tag
    TAG_MSG_READ_REQUEST,           // Optional: async read (future)
} tag_message_type_t;

/**
 * @brief Message sent to tag database task
 */
typedef struct {
    tag_message_type_t msg_type;
    tag_handle_t tag_handle;
    tag_quality_t quality;
    tag_value_t value;
} tag_message_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * @brief Initialize tag database system
 * 
 * Creates the tag database task, queue, and mutex.
 * Call once during system initialization.
 * 
 * @return true if successful, false on error
 */
bool tag_db_init(void);

/**
 * @brief Create a new tag
 * 
 * Allocates a tag and assigns it a name and type.
 * Call during system initialization to define all tags.
 * 
 * @param name Tag name (max 15 chars + null terminator)
 * @param data_type Data type for this tag
 * @return Tag handle (0-127), or TAG_HANDLE_INVALID on error
 */
tag_handle_t tag_db_create(const char *name, tag_data_type_t data_type);

/**
 * @brief Get tag handle by name
 * 
 * Resolve tag name to handle. Call once at task initialization,
 * then use the handle for all subsequent operations.
 * 
 * @param name Tag name to look up
 * @return Tag handle, or TAG_HANDLE_INVALID if not found
 */
tag_handle_t tag_db_get_handle(const char *name);

/**
 * @brief Get tag name by handle (for debugging/logging)
 * 
 * @param handle Tag handle
 * @param name_out Buffer to store name (min TAG_NAME_MAX_LEN bytes)
 * @return true if successful, false if invalid handle
 */
bool tag_db_get_name(tag_handle_t handle, char *name_out);

/**
 * @brief Write value to tag (non-blocking)
 * 
 * Sends write message to tag database queue. Returns immediately.
 * User must cast value to correct type based on tag definition.
 * 
 * Example:
 *   tag_value_t value;
 *   value.float_val = 25.3;
 *   tag_db_write(temp_handle, value, TAG_QUALITY_GOOD);
 * 
 * @param handle Tag handle
 * @param value Value to write (union, user casts to correct type)
 * @param quality Quality indicator (GOOD, BAD, UNCERTAIN)
 * @return true if message queued, false if queue full or invalid handle
 */
bool tag_db_write(tag_handle_t handle, tag_value_t value, tag_quality_t quality);

/**
 * @brief Read value from tag (blocking with mutex)
 * 
 * Reads current value from tag. User must cast value from union.
 * 
 * Example:
 *   tag_value_t value;
 *   tag_quality_t quality;
 *   uint32_t timestamp;
 *   
 *   if (tag_db_read(temp_handle, &value, &quality, &timestamp)) {
 *       float temperature = value.float_val;
 *       printf("Temp: %.1f, Quality: %d\n", temperature, quality);
 *   }
 * 
 * @param handle Tag handle
 * @param value_out Pointer to receive value (required)
 * @param quality_out Pointer to receive quality (NULL if not needed)
 * @param timestamp_out Pointer to receive timestamp (NULL if not needed)
 * @return true if successful, false if invalid handle
 */
bool tag_db_read(tag_handle_t handle, tag_value_t *value_out,
                 tag_quality_t *quality_out, uint32_t *timestamp_out);

/**
 * @brief Get tag metadata by handle
 * 
 * Returns complete metadata including name, type, current value, etc.
 * Useful for generating tag lists in web interface.
 * 
 * @param handle Tag handle
 * @param metadata_out Pointer to receive metadata
 * @return true if successful, false if invalid handle
 */
bool tag_db_get_metadata(tag_handle_t handle, tag_metadata_t *metadata_out);

/**
 * @brief Get number of allocated tags
 * 
 * @return Count of tags created via tag_db_create()
 */
uint16_t tag_db_get_tag_count(void);

#endif // _TAG_DATABASE_H_
