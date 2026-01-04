add_library(pico_flash_storage INTERFACE)

target_sources(pico_flash_storage INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/pico_flash_storage.c
)

target_include_directories(pico_flash_storage INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico_flash_storage INTERFACE
    pico_stdlib
    pico_multicore
    hardware_flash
    hardware_sync
    FreeRTOS-Kernel
    freertos_config
)
