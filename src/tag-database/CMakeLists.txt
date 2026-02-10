add_library(tag-database INTERFACE)

target_sources(tag-database INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/tag_database.c
)

target_include_directories(tag-database INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(tag-database INTERFACE
    pico_stdlib
    FreeRTOS-Kernel
    logger
    system_config
)
