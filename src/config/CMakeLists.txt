add_library(config INTERFACE)

target_sources(config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/config.c
)

target_include_directories(config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(config INTERFACE
    pico_stdlib
    hardware_flash
    pico_flash_storage
)