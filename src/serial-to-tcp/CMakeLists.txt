add_library(serial_to_tcp INTERFACE)

target_sources(serial_to_tcp INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/serial_to_tcp.c
)

target_include_directories(serial_to_tcp INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(serial_to_tcp INTERFACE
    pico_stdlib
    hardware_uart
    FreeRTOS-Kernel
    IOLIBRARY_FILES
    system_config
    board_config
    logger
)
