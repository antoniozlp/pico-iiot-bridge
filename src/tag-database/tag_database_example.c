/**
 * @file tag_database_example.c
 * @brief Example usage of tag database
 * 
 * This file demonstrates how to use the tag database for producer-consumer
 * communication. It includes example producer and consumer tasks.
 * 
 * To use this example:
 * 1. Add this file to your CMakeLists.txt
 * 2. Call example_producer_task_init() and example_consumer_task_init() in main.c
 */

#include "tag_database_example.h"

#include "tag_database.h"
#include "logger.h"
#include "FreeRTOS.h"
#include "task.h"

// ============================================================================
// Example Tag Creation
// Creates the demo tags used by the producer/consumer example tasks
// ============================================================================

bool example_tags_init(void)
{
    // Skip creation for tags that already exist (e.g. persisted from flash
    // under the same name)
    if (tag_db_get_handle("TEMP_SENSOR_01") == TAG_HANDLE_INVALID &&
        tag_db_create("TEMP_SENSOR_01", TAG_TYPE_FLOAT) == TAG_HANDLE_INVALID)
    {
        LOG_ERROR("Failed to create TEMP_SENSOR_01 tag");
        return false;
    }

    if (tag_db_get_handle("PRESSURE_01") == TAG_HANDLE_INVALID &&
        tag_db_create("PRESSURE_01", TAG_TYPE_UINT16) == TAG_HANDLE_INVALID)
    {
        LOG_ERROR("Failed to create PRESSURE_01 tag");
        return false;
    }

    if (tag_db_get_handle("MOTOR_RUNNING") == TAG_HANDLE_INVALID &&
        tag_db_create("MOTOR_RUNNING", TAG_TYPE_BOOL) == TAG_HANDLE_INVALID)
    {
        LOG_ERROR("Failed to create MOTOR_RUNNING tag");
        return false;
    }

    return true;
}

// ============================================================================
// Example Producer Task
// Simulates a sensor reading and writing to tag database
// ============================================================================

static tag_handle_t s_example_temp_handle;
static tag_handle_t s_example_pressure_handle;

static void vExampleProducerTask(void *pvParameters)
{
    (void)pvParameters;
    
    float temperature = 20.0;
    uint16_t pressure = 1000;
    
    LOG_INFO("Example producer task started");
    
    while (1)
    {
        // Simulate temperature sensor reading (incrementing for demo)
        temperature += 0.5;
        if (temperature > 30.0)
        {
            temperature = 20.0;
        }
        
        // Write temperature to tag database
        tag_value_t temp_value;
        temp_value.float_val = temperature;
        
        if (tag_db_write(s_example_temp_handle, temp_value, TAG_QUALITY_GOOD))
        {
            LOG_DEBUG("Producer wrote temperature: %.1f", temperature);
        }
        else
        {
            LOG_ERROR("Failed to write temperature tag");
        }
        
        // Simulate pressure sensor reading
        pressure += 10;
        if (pressure > 1100)
        {
            pressure = 1000;
        }
        
        // Write pressure to tag database
        tag_value_t pressure_value;
        pressure_value.u16_val = pressure;
        
        if (tag_db_write(s_example_pressure_handle, pressure_value, TAG_QUALITY_GOOD))
        {
            LOG_DEBUG("Producer wrote pressure: %u", pressure);
        }
        else
        {
            LOG_ERROR("Failed to write pressure tag");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Update every 10ms
    }
}

bool example_producer_task_init(void)
{
    // Resolve tag handles (assumes tags were created in main.c)
    s_example_temp_handle = tag_db_get_handle("TEMP_SENSOR_01");
    s_example_pressure_handle = tag_db_get_handle("PRESSURE_01");
    
    if (s_example_temp_handle == TAG_HANDLE_INVALID || 
        s_example_pressure_handle == TAG_HANDLE_INVALID)
    {
        LOG_ERROR("Failed to resolve example tag handles");
        return false;
    }
    
    BaseType_t result = xTaskCreate(
        vExampleProducerTask,
        "ExProd",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
    
    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create example producer task");
        return false;
    }
    
    LOG_INFO("Example producer task initialized");
    return true;
}

// ============================================================================
// Example Consumer Task
// Reads values from tag database and displays them
// ============================================================================

static tag_handle_t s_consumer_temp_handle;
static tag_handle_t s_consumer_pressure_handle;
static tag_handle_t s_consumer_motor_handle;

static void vExampleConsumerTask(void *pvParameters)
{
    (void)pvParameters;
    
    LOG_INFO("Example consumer task started");
    
    while (1)
    {
        tag_value_t value;
        tag_quality_t quality;
        uint32_t timestamp;
        
        // Read temperature
        if (tag_db_read(s_consumer_temp_handle, &value, &quality, &timestamp))
        {
            if (quality == TAG_QUALITY_GOOD)
            {
                LOG_INFO("Temperature: %.1f °C (updated %lu ms ago)", 
                         value.float_val, 
                         (xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);
            }
            else
            {
                LOG_WARN("Temperature quality: %d", quality);
            }
        }
        else
        {
            LOG_ERROR("Failed to read temperature tag");
        }
        
        // Read pressure
        if (tag_db_read(s_consumer_pressure_handle, &value, &quality, &timestamp))
        {
            if (quality == TAG_QUALITY_GOOD)
            {
                LOG_INFO("Pressure: %u mbar", value.u16_val);
            }
        }
        
        // Read motor status
        if (tag_db_read(s_consumer_motor_handle, &value, &quality, NULL))
        {
            LOG_INFO("Motor: %s", value.bool_val ? "RUNNING" : "STOPPED");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));  // Display every 2 seconds
    }
}

bool example_consumer_task_init(void)
{
    // Resolve tag handles
    s_consumer_temp_handle = tag_db_get_handle("TEMP_SENSOR_01");
    s_consumer_pressure_handle = tag_db_get_handle("PRESSURE_01");
    s_consumer_motor_handle = tag_db_get_handle("MOTOR_RUNNING");
    
    if (s_consumer_temp_handle == TAG_HANDLE_INVALID || 
        s_consumer_pressure_handle == TAG_HANDLE_INVALID ||
        s_consumer_motor_handle == TAG_HANDLE_INVALID)
    {
        LOG_ERROR("Failed to resolve consumer tag handles");
        return false;
    }
    
    BaseType_t result = xTaskCreate(
        vExampleConsumerTask,
        "ExCons",
        configMINIMAL_STACK_SIZE * 3,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    
    if (result != pdPASS)
    {
        LOG_ERROR("Failed to create example consumer task");
        return false;
    }
    
    LOG_INFO("Example consumer task initialized");
    return true;
}
