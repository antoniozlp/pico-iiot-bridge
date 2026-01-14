add_library(logger INTERFACE)

target_sources(logger INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/logger.c
)

target_include_directories(logger INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(logger INTERFACE
    pico_stdlib
    FreeRTOS-Kernel
)
