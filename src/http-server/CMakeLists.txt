add_library(http_server STATIC)

target_sources(http_server PUBLIC
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
        )
