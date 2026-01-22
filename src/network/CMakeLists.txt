add_library(network_task STATIC)

target_sources(network_task PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/network_task.c
    ${CMAKE_SOURCE_DIR}/lib/WIZnet/ioLibrary_Driver/Internet/DHCP/dhcp.c
)

target_include_directories(network_task PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/WIZnet/ioLibrary_Driver/Internet/DHCP
    ${CMAKE_SOURCE_DIR}/lib/WIZnet/port/timer
)

target_link_libraries(network_task PUBLIC
    pico_stdlib
    FreeRTOS-Kernel
    system_config
    logger
    board_config
    IOLIBRARY_FILES
    TIMER_FILES
)
