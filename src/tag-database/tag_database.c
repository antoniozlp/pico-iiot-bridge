/**
 * @file tag_database.c
 * @brief Tag database implementation
 */

#include "tag_database.h"
#include "logger.h"
#include "semphr.h"
#include "system_config.h"
#include <string.h>

#define TAG_DB_MUTEX_TIMEOUT_MS 100

// ============================================================================
// Internal Storage
// ============================================================================

// Tag metadata array (all tags stored here)
static tag_metadata_t s_tags[TAG_DATABASE_MAX_TAGS];

// Highest allocated handle + 1 (not the count of active tags)
static uint16_t s_tag_high_water_mark = 0;

// FreeRTOS primitives
static QueueHandle_t s_tag_db_queue = NULL;
static SemaphoreHandle_t s_tag_db_mutex = NULL;
static TaskHandle_t s_tag_db_task_handle = NULL;

// ============================================================================
// Internal Functions
// ============================================================================

/**
 * @brief Tag database management task
 */
static void vTagDatabaseTask(void *pvParameters)
{
    (void)pvParameters;
    
    tag_message_t msg;
    
    LOG_INFO("Tag database task started");
    
    while (1)
    {
        // Wait for incoming write messages
        if (xQueueReceive(s_tag_db_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            // Validate handle against array bounds (compile-time constant, no mutex needed)
            if (msg.tag_handle >= TAG_DATABASE_MAX_TAGS)
            {
                LOG_ERROR("Invalid tag handle (out of bounds): %d", msg.tag_handle);
                continue;
            }
            
            // Acquire mutex before accessing shared state
            if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
            {
                LOG_ERROR("Failed to acquire tag database mutex");
                continue;
            }
            
            // Validate handle against high water mark (inside mutex)
            if (msg.tag_handle >= s_tag_high_water_mark)
            {
                xSemaphoreGive(s_tag_db_mutex);
                LOG_ERROR("Invalid tag handle (not yet allocated): %d", msg.tag_handle);
                continue;
            }
            
            // Check if tag is still allocated
            if (!s_tags[msg.tag_handle].allocated)
            {
                xSemaphoreGive(s_tag_db_mutex);
                LOG_WARN("Write to deleted tag handle: %d", msg.tag_handle);
                continue;
            }
            
            // Process message
            switch (msg.msg_type)
            {
                case TAG_MSG_WRITE:
                {
                    tag_metadata_t *tag = &s_tags[msg.tag_handle];
                    
                    // Update value, quality, and timestamp
                    tag->value = msg.value;
                    tag->quality = msg.quality;
                    tag->timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
                    
                    LOG_DEBUG("Tag updated: %s (handle=%d)", tag->name, msg.tag_handle);
                    break;
                }
                
                case TAG_MSG_READ_REQUEST:
                    // Future: async read support
                    break;
                    
                default:
                    LOG_ERROR("Unknown message type: %d", msg.msg_type);
                    break;
            }
            
            xSemaphoreGive(s_tag_db_mutex);
        }
    }
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool tag_db_init(void)
{
    // Initialize tag array (all tags marked as not allocated)
    memset(s_tags, 0, sizeof(s_tags));
    s_tag_high_water_mark = 0;
    
    // Create mutex
    s_tag_db_mutex = xSemaphoreCreateMutex();
    if (s_tag_db_mutex == NULL)
    {
        LOG_ERROR("Failed to create tag database mutex");
        return false;
    }
    
    // Create queue
    s_tag_db_queue = xQueueCreate(TAG_DB_QUEUE_LENGTH, sizeof(tag_message_t));
    if (s_tag_db_queue == NULL)
    {
        LOG_ERROR("Failed to create tag database queue");
        vSemaphoreDelete(s_tag_db_mutex);
        s_tag_db_mutex = NULL;
        return false;
    }
    
    // Create task
    BaseType_t result = xTaskCreate(
        vTagDatabaseTask,
        "TagDB",
        TAG_DB_TASK_STACK_SIZE,
        NULL,
        TAG_DB_TASK_PRIORITY,
        &s_tag_db_task_handle
    );
    
    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create tag database task");
        vQueueDelete(s_tag_db_queue);
        s_tag_db_queue = NULL;
        vSemaphoreDelete(s_tag_db_mutex);
        s_tag_db_mutex = NULL;
        return false;
    }
    
    LOG_INFO("Tag database initialized");
    return true;
}

tag_handle_t tag_db_create(const char *name, tag_data_type_t data_type)
{
    if (name == NULL || data_type > TAG_TYPE_FLOAT)
    {
        LOG_ERROR("Invalid parameters for tag_db_create");
        return TAG_HANDLE_INVALID;
    }
    
    // Acquire mutex first to prevent TOCTOU race condition
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        LOG_ERROR("Failed to acquire mutex for tag creation");
        return TAG_HANDLE_INVALID;
    }
    
    // Check for duplicate name among allocated tags (inside mutex)
    for (uint16_t i = 0; i < s_tag_high_water_mark; i++)
    {
        if (s_tags[i].allocated && strcmp(s_tags[i].name, name) == 0)
        {
            xSemaphoreGive(s_tag_db_mutex);
            LOG_ERROR("Tag already exists: %s", name);
            return TAG_HANDLE_INVALID;
        }
    }
    
    // Try to reuse a deleted slot first
    tag_handle_t handle = TAG_HANDLE_INVALID;
    for (uint16_t i = 0; i < s_tag_high_water_mark; i++)
    {
        if (!s_tags[i].allocated)
        {
            handle = i;
            break;
        }
    }
    
    // If no reusable slot, allocate new one
    if (handle == TAG_HANDLE_INVALID)
    {
        if (s_tag_high_water_mark >= TAG_DATABASE_MAX_TAGS)
        {
            xSemaphoreGive(s_tag_db_mutex);
            LOG_ERROR("Tag database full (max=%d)", TAG_DATABASE_MAX_TAGS);
            return TAG_HANDLE_INVALID;
        }
        handle = s_tag_high_water_mark;
        s_tag_high_water_mark++;
    }
    
    // Initialize tag
    tag_metadata_t *tag = &s_tags[handle];
    strncpy(tag->name, name, TAG_NAME_MAX_LEN - 1);
    tag->name[TAG_NAME_MAX_LEN - 1] = '\0';
    tag->data_type = data_type;
    tag->quality = TAG_QUALITY_UNCERTAIN;  // No value yet
    tag->timestamp_ms = 0;
    memset(&tag->value, 0, sizeof(tag_value_t));
    tag->allocated = 1;  // Mark as active
    
    xSemaphoreGive(s_tag_db_mutex);
    
    LOG_INFO("Tag created: %s (handle=%d, type=%d)", name, handle, data_type);
    return handle;
}

tag_handle_t tag_db_get_handle(const char *name)
{
    if (name == NULL)
    {
        return TAG_HANDLE_INVALID;
    }
    
    // Linear search through allocated tags only
    for (uint16_t i = 0; i < s_tag_high_water_mark; i++)
    {
        if (s_tags[i].allocated && strcmp(s_tags[i].name, name) == 0)
        {
            return (tag_handle_t)i;
        }
    }
    
    return TAG_HANDLE_INVALID;
}

bool tag_db_get_name(tag_handle_t handle, char *name_out)
{
    if (handle >= s_tag_high_water_mark || name_out == NULL || !s_tags[handle].allocated)
    {
        return false;
    }
    
    strncpy(name_out, s_tags[handle].name, TAG_NAME_MAX_LEN);
    return true;
}

bool tag_db_write(tag_handle_t handle, tag_value_t value, tag_quality_t quality)
{
    if (handle >= s_tag_high_water_mark || !s_tags[handle].allocated)
    {
        return false;
    }
    
    // Build message
    tag_message_t msg = {
        .msg_type = TAG_MSG_WRITE,
        .tag_handle = handle,
        .quality = quality,
        .value = value
    };
    
    // Send to queue (non-blocking)
    if (xQueueSend(s_tag_db_queue, &msg, 0) != pdTRUE)
    {
        LOG_WARN("Tag database queue full, dropping update for handle %d", handle);
        return false;
    }
    
    return true;
}

bool tag_db_read(tag_handle_t handle, tag_value_t *value_out,
                 tag_quality_t *quality_out, uint32_t *timestamp_out)
{
    if (handle >= s_tag_high_water_mark || value_out == NULL)
    {
        return false;
    }
    
    // Acquire mutex
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }
    
    // Check if tag is still allocated
    if (!s_tags[handle].allocated)
    {
        xSemaphoreGive(s_tag_db_mutex);
        return false;
    }
    
    // Read values
    tag_metadata_t *tag = &s_tags[handle];
    *value_out = tag->value;
    
    if (quality_out != NULL)
    {
        *quality_out = tag->quality;
    }
    
    if (timestamp_out != NULL)
    {
        *timestamp_out = tag->timestamp_ms;
    }
    
    xSemaphoreGive(s_tag_db_mutex);
    return true;
}

bool tag_db_get_metadata(tag_handle_t handle, tag_metadata_t *metadata_out)
{
    if (handle >= s_tag_high_water_mark || metadata_out == NULL)
    {
        return false;
    }
    
    // Acquire mutex
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }
    
    // Check if tag is allocated
    if (!s_tags[handle].allocated)
    {
        xSemaphoreGive(s_tag_db_mutex);
        return false;
    }
    
    // Copy entire metadata structure
    memcpy(metadata_out, &s_tags[handle], sizeof(tag_metadata_t));
    
    xSemaphoreGive(s_tag_db_mutex);
    return true;
}

uint16_t tag_db_get_tag_count(void)
{
    uint16_t count = 0;
    for (uint16_t i = 0; i < s_tag_high_water_mark; i++)
    {
        if (s_tags[i].allocated)
        {
            count++;
        }
    }
    return count;
}

// ============================================================================
// Persistence Functions
// ============================================================================

/**
 * @brief Load tag definitions from flash and create tags
 * 
 * CRITICAL: Loads tags directly at their flash index (not via tag_db_create)
 * to ensure stable handle-to-index mapping across reboots. Flash index i
 * always maps to runtime handle i for persistent tags.
 */
uint16_t tag_db_load_from_flash(void)
{
    tag_database_config_t config;
    
    if (!config_get_tag_database(&config))
    {
        LOG_ERROR("Failed to load tag database config from flash");
        return 0;
    }
    
    // Acquire mutex for safe array manipulation
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        LOG_ERROR("Failed to acquire mutex for tag loading");
        return 0;
    }
    
    uint16_t created_count = 0;
    
    // Load tags directly at their flash index (preserves handle stability)
    for (uint16_t i = 0; i < TAG_DB_MAX_PERSISTENT_TAGS; i++)
    {
        if (config.tags[i].enabled)
        {
            tag_data_type_t data_type = (tag_data_type_t)config.tags[i].data_type;
            
            // Validate data type
            if (data_type > TAG_TYPE_FLOAT)
            {
                LOG_WARN("Invalid data type for tag from flash: %s (type=%d)", 
                         config.tags[i].name, data_type);
                continue;
            }
            
            // Load directly at index i (same as flash position)
            tag_metadata_t *tag = &s_tags[i];
            strncpy(tag->name, config.tags[i].name, TAG_NAME_MAX_LEN - 1);
            tag->name[TAG_NAME_MAX_LEN - 1] = '\0';
            tag->data_type = data_type;
            tag->quality = TAG_QUALITY_UNCERTAIN;  // No value yet
            tag->timestamp_ms = 0;
            memset(&tag->value, 0, sizeof(tag_value_t));
            tag->allocated = 1;  // Mark as active
            
            created_count++;
            LOG_DEBUG("Loaded tag from flash: %s (handle=%d, type=%d)", 
                     config.tags[i].name, i, data_type);
        }
        // If not enabled, leave slot unallocated (preserve gap for stability)
    }
    
    // Set high water mark to cover all persistent tag slots
    // This reserves handles 0-63 for persistent tags, 64-127 for runtime tags
    if (s_tag_high_water_mark < TAG_DB_MAX_PERSISTENT_TAGS)
    {
        s_tag_high_water_mark = TAG_DB_MAX_PERSISTENT_TAGS;
    }
    
    xSemaphoreGive(s_tag_db_mutex);
    
    LOG_INFO("Loaded %d tags from flash (handles 0-%d reserved for persistence)", 
             created_count, TAG_DB_MAX_PERSISTENT_TAGS - 1);
    return created_count;
}

/**
 * @brief Save current tag definitions to flash
 * 
 * CRITICAL: Saves tags at their handle index (no compacting) to ensure
 * stable handle-to-index mapping across reboots. Only persistent tags
 * (handles 0-63) are saved. Runtime-only tags (handles 64-127) are not persisted.
 */
bool tag_db_save_to_flash(void)
{
    tag_database_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.auto_create_enabled = 1;
    config.reserved = 0;
    
    uint16_t saved_count = 0;
    
    // Save tags at their handle index (preserve gaps for stability)
    // Only save persistent zone (handles 0-63)
    for (uint16_t i = 0; i < TAG_DB_MAX_PERSISTENT_TAGS; i++)
    {
        if (s_tags[i].allocated)
        {
            // Save at index i (same as handle)
            strncpy(config.tags[i].name, s_tags[i].name, TAG_NAME_MAX_LEN - 1);
            config.tags[i].name[TAG_NAME_MAX_LEN - 1] = '\0';
            config.tags[i].data_type = (uint8_t)s_tags[i].data_type;
            config.tags[i].enabled = 1;
            memset(config.tags[i].reserved, 0, sizeof(config.tags[i].reserved));
            saved_count++;
        }
        else
        {
            // Mark slot as deleted (preserve gap)
            config.tags[i].enabled = 0;
            memset(config.tags[i].name, 0, TAG_NAME_MAX_LEN);
            config.tags[i].data_type = 0;
            memset(config.tags[i].reserved, 0, sizeof(config.tags[i].reserved));
        }
    }
    
    config.tag_count = saved_count;
    
    // Save to flash via config system
    if (!config_set_tag_database(&config))
    {
        LOG_ERROR("Failed to set tag database config");
        return false;
    }
    
    // Trigger flash write
    if (!config_save_to_flash())
    {
        LOG_ERROR("Failed to save tag database to flash");
        return false;
    }
    
    LOG_INFO("Saved %d tags to flash (handles 0-%d, preserving gaps for stability)", 
             saved_count, TAG_DB_MAX_PERSISTENT_TAGS - 1);
    return true;
}

/**
 * @brief Create tag and optionally save to flash
 */
tag_handle_t tag_db_create_persistent(const char *name, tag_data_type_t data_type, bool persist)
{
    // Create tag in runtime database
    tag_handle_t handle = tag_db_create(name, data_type);
    
    if (handle == TAG_HANDLE_INVALID)
    {
        return TAG_HANDLE_INVALID;
    }
    
    // Optionally persist to flash
    if (persist)
    {
        if (!config_add_tag_definition(name, (uint8_t)data_type))
        {
            LOG_WARN("Tag created in RAM but failed to persist: %s", name);
            // Don't fail - tag still exists in RAM
        }
        else
        {
            if (!config_save_to_flash())
            {
                LOG_ERROR("Tag added to config but flash write failed: %s", name);
                // Tag exists in RAM but may not survive reboot
            }
            else
            {
                LOG_INFO("Tag persisted to flash: %s", name);
            }
        }
    }
    
    return handle;
}

/**
 * @brief Delete tag by name
 * 
 * Marks tag as deleted rather than compacting the array.
 * This preserves handle validity for all other tags.
 * The slot can be reused by future tag_db_create() calls.
 */
bool tag_db_delete(const char *name, bool persist)
{
    if (name == NULL)
    {
        return false;
    }
    
    // Acquire mutex for safe deletion
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        LOG_ERROR("Failed to acquire mutex for tag deletion");
        return false;
    }
    
    // Find tag by name among allocated tags
    tag_handle_t handle = TAG_HANDLE_INVALID;
    for (uint16_t i = 0; i < s_tag_high_water_mark; i++)
    {
        if (s_tags[i].allocated && strcmp(s_tags[i].name, name) == 0)
        {
            handle = i;
            break;
        }
    }
    
    if (handle == TAG_HANDLE_INVALID)
    {
        xSemaphoreGive(s_tag_db_mutex);
        LOG_WARN("Tag not found for deletion: %s", name);
        return false;
    }
    
    // Mark tag as deleted (handle remains valid but unusable)
    // This prevents cached handles from becoming stale
    s_tags[handle].allocated = 0;
    memset(&s_tags[handle].value, 0, sizeof(tag_value_t));
    s_tags[handle].quality = TAG_QUALITY_BAD;
    s_tags[handle].timestamp_ms = 0;
    
    xSemaphoreGive(s_tag_db_mutex);
    
    LOG_INFO("Tag marked as deleted: %s (handle=%d, slot can be reused)", name, handle);
    
    // Optionally remove from flash
    if (persist)
    {
        if (config_remove_tag_definition(name))
        {
            if (!config_save_to_flash())
            {
                LOG_ERROR("Tag removed from config but flash write failed: %s", name);
                // Tag deleted from RAM but old definition may persist in flash after reboot
            }
            else
            {
                LOG_INFO("Tag removed from flash: %s", name);
            }
        }
        else
        {
            LOG_WARN("Tag deleted from RAM but not found in flash: %s", name);
        }
    }
    
    return true;
}
