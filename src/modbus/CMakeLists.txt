add_library(modbus INTERFACE)

target_sources(modbus INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modbus_rtu.c)

target_include_directories(modbus INTERFACE
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(modbus INTERFACE
    pico_stdlib
    FreeRTOS-Kernel
    nanomodbus
    logger)