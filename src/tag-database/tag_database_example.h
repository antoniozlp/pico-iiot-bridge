/**
 * @file tag_database_example.h
 * @brief Example producer/consumer tasks demonstrating tag database usage
 */

#ifndef _TAG_DATABASE_EXAMPLE_H_
#define _TAG_DATABASE_EXAMPLE_H_

#include <stdbool.h>

/**
 * @brief Create the demo tags used by the example producer/consumer tasks
 *
 * Creates TEMP_SENSOR_01 (FLOAT), PRESSURE_01 (UINT16), and MOTOR_RUNNING
 * (BOOL). Skips creation for any tag that already exists (e.g. persisted
 * from flash under the same name). Call after tag_db_init() and before
 * example_producer_task_init() / example_consumer_task_init().
 *
 * @return true if all tags exist or were created successfully, false on error
 */
bool example_tags_init(void);

/**
 * @brief Initialize the example producer task
 *
 * Resolves TEMP_SENSOR_01 and PRESSURE_01 tag handles (must already exist)
 * and starts a task that periodically writes simulated sensor values.
 *
 * @return true if successful, false if tags are missing or task creation failed
 */
bool example_producer_task_init(void);

/**
 * @brief Initialize the example consumer task
 *
 * Resolves TEMP_SENSOR_01, PRESSURE_01, and MOTOR_RUNNING tag handles (must
 * already exist) and starts a task that periodically reads and logs them.
 *
 * @return true if successful, false if tags are missing or task creation failed
 */
bool example_consumer_task_init(void);

#endif /* _TAG_DATABASE_EXAMPLE_H_ */
