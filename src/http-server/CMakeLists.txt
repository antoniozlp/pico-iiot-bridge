add_library(http_server STATIC)

target_sources(http_server PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/http_utils.c
        ${CMAKE_SOURCE_DIR}/lib/WIZnet/ioLibrary_Driver/Internet/httpServer/httpServer.c
        ${CMAKE_SOURCE_DIR}/lib/WIZnet/ioLibrary_Driver/Internet/httpServer/httpParser.c
        )

target_include_directories(http_server PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_SOURCE_DIR}/lib/WIZnet/ioLibrary_Driver/Internet/httpServer
        )

target_link_libraries(http_server PUBLIC
        pico_stdlib
        ETHERNET_FILES
        config
        logger
        )

# Disable HTTP server debug messages from WIZnet library
# Use -include to force our header before httpServer.c compiles
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/lib/WIZnet/ioLibrary_Driver/Internet/httpServer/httpServer.c
    PROPERTIES
    COMPILE_FLAGS "-include ${CMAKE_CURRENT_LIST_DIR}/http_utils.h"
)
