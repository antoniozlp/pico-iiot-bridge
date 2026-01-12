# RTOS Views Support Library
# Provides runtime statistics for FreeRTOS when RTOS_VIEWS_SUPPORT is enabled

add_library(rtos_views_support INTERFACE)

target_sources(rtos_views_support INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/rtos_views_support.c
)

target_include_directories(rtos_views_support INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(rtos_views_support INTERFACE
    pico_stdlib
    pico_time
    FreeRTOS-Kernel
)
