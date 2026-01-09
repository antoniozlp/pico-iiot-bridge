add_library(board_config INTERFACE)

target_sources(board_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/board_config.c
)

target_include_directories(board_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(board_config INTERFACE
    pico_stdlib
    hardware_gpio
    hardware_spi
    hardware_i2c
    hardware_uart
    system_config
)