add_library(FreeRTOS-Plus-CLI INTERFACE)

target_sources(FreeRTOS-Plus-CLI INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS_CLI.c
)

target_include_directories(FreeRTOS-Plus-CLI INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(FreeRTOS-Plus-CLI INTERFACE
    FreeRTOS-Kernel
    freertos_config
)
