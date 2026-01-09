# Add subdirectories for system and board configuration
add_subdirectory(system_config)
add_subdirectory(board_config)

# Create a top-level config library that includes both
add_library(config INTERFACE)

target_link_libraries(config INTERFACE
    system_config
    board_config
)
