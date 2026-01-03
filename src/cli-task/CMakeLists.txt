add_library(cli-task INTERFACE)

target_sources(cli-task INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/cli_task.c
)

target_include_directories(cli-task INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(cli-task INTERFACE
    pico_stdlib
    hardware_watchdog
    FreeRTOS-Kernel
    FreeRTOS-Plus-CLI
)

