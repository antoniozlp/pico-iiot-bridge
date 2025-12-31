add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Also update the FREERTOS_CONFIG_FILE_DIRECTORY variable if it's already set
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "Path to FreeRTOSConfig.h")
