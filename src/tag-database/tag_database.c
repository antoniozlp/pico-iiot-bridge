/**
 * @file tag_database.c
 * @brief Tag database implementation
 */

#include "tag_database.h"
#include "logger.h"
#include "semphr.h"
#include <string.h>

#define TAG_DB_MUTEX_TIMEOUT_MS 100

// ============================================================================
// Internal Storage
// ============================================================================

// Tag metadata array (all tags stored here)
static tag_metadata_t s_tags[TAG_DATABASE_MAX_TAGS];

// Number of allocated tags
static uint16_t s_tag_count = 0;

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
            // Validate handle
            if (msg.tag_handle >= TAG_DATABASE_MAX_TAGS || msg.tag_handle >= s_tag_count)
            {
                LOG_ERROR("Invalid tag handle: %d", msg.tag_handle);
                continue;
            }
            
            // Acquire mutex for atomic update
            if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
            {
                LOG_ERROR("Failed to acquire tag database mutex");
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
    // Initialize tag array
    memset(s_tags, 0, sizeof(s_tags));
    s_tag_count = 0;
    
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
        vSemaphoreDelete(s_tag_db_mutex);
        return false;
    }
    
    LOG_INFO("Tag database initialized");
    return true;
}

tag_handle_t tag_db_create(const char *name, tag_data_type_t data_type)
{
    if (name == NULL || data_type >= TAG_TYPE_COUNT)
    {
        LOG_ERROR("Invalid parameters for tag_db_create");
        return TAG_HANDLE_INVALID;
    }
    
    // Check if we have space
    if (s_tag_count >= TAG_DATABASE_MAX_TAGS)
    {
        LOG_ERROR("Tag database full (max=%d)", TAG_DATABASE_MAX_TAGS);
        return TAG_HANDLE_INVALID;
    }
    
    // Check for duplicate name
    for (uint16_t i = 0; i < s_tag_count; i++)
    {
        if (strcmp(s_tags[i].name, name) == 0)
        {
            LOG_ERROR("Tag already exists: %s", name);
            return TAG_HANDLE_INVALID;
        }
    }
    
    // Acquire mutex
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        LOG_ERROR("Failed to acquire mutex for tag creation");
        return TAG_HANDLE_INVALID;
    }
    
    // Allocate tag
    tag_handle_t handle = s_tag_count;
    tag_metadata_t *tag = &s_tags[handle];
    
    strncpy(tag->name, name, TAG_NAME_MAX_LEN - 1);
    tag->name[TAG_NAME_MAX_LEN - 1] = '\0';
    tag->data_type = data_type;
    tag->quality = TAG_QUALITY_UNCERTAIN;  // No value yet
    tag->timestamp_ms = 0;
    memset(&tag->value, 0, sizeof(tag_value_t));
    
    s_tag_count++;
    
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
    
    // Linear search (no mutex needed for read-only access to names)
    for (uint16_t i = 0; i < s_tag_count; i++)
    {
        if (strcmp(s_tags[i].name, name) == 0)
        {
            return (tag_handle_t)i;
        }
    }
    
    return TAG_HANDLE_INVALID;
}

bool tag_db_get_name(tag_handle_t handle, char *name_out)
{
    if (handle >= s_tag_count || name_out == NULL)
    {
        return false;
    }
    
    strncpy(name_out, s_tags[handle].name, TAG_NAME_MAX_LEN);
    return true;
}

bool tag_db_write(tag_handle_t handle, tag_value_t value, tag_quality_t quality)
{
    if (handle >= s_tag_count)
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
    if (handle >= s_tag_count || value_out == NULL)
    {
        return false;
    }
    
    // Acquire mutex
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
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
    if (handle >= s_tag_count || metadata_out == NULL)
    {
        return false;
    }
    
    // Acquire mutex
    if (xSemaphoreTake(s_tag_db_mutex, pdMS_TO_TICKS(TAG_DB_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return false;
    }
    
    // Copy entire metadata structure
    memcpy(metadata_out, &s_tags[handle], sizeof(tag_metadata_t));
    
    xSemaphoreGive(s_tag_db_mutex);
    return true;
}

uint16_t tag_db_get_tag_count(void)
{
    return s_tag_count;
}
