add_library(system_config INTERFACE)

target_sources(system_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/system_config.c
)

target_include_directories(system_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(system_config INTERFACE
    pico_stdlib
    hardware_flash
    pico_flash_storage
    logger
)
