add_library(modbus INTERFACE)

target_sources(modbus INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modbus_rtu_master.c
    ${CMAKE_CURRENT_LIST_DIR}/modbus_rtu_slave.c
    ${CMAKE_CURRENT_LIST_DIR}/modbus_tcp_client.c
    ${CMAKE_CURRENT_LIST_DIR}/modbus_tag_mapping.c
    ${CMAKE_CURRENT_LIST_DIR}/modbus_request_processor.c)

target_include_directories(modbus INTERFACE
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(modbus INTERFACE
    pico_stdlib
    FreeRTOS-Kernel
    nanomodbus
    logger
    tag-database
    system_config
    ETHERNET_FILES
    network_task)
