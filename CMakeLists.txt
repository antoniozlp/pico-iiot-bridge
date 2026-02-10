# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-iiot-bridge C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


# ========================= FreeRTOS =========================
# Set the FreeRTOS Kernel path to the local FreeRTOS-Kernel folder
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)

# Enable RTOS Views support for VS Code debugging by default
# Comment out the following line to disable runtime statistics in production builds
add_definitions(-DRTOS_VIEWS_SUPPORT)

# Add FreeRTOS configuration (must be before FreeRTOS kernel import)
add_subdirectory(lib/FreeRTOS-config)
# Pull in FreeRTOS Kernel
include(FreeRTOS_Kernel_import.cmake)
# ===========================================================

# ========================= WIZnet =========================
# Set board
# set(BOARD_NAME WIZnet_Ethernet_HAT)
# set(BOARD_NAME W5100S_EVB_PICO)
# set(BOARD_NAME W5500_EVB_PICO)
# set(BOARD_NAME W55RP20_EVB_PICO)
# set(BOARD_NAME W6100_EVB_PICO)
# set(BOARD_NAME W6300_EVB_PICO)
# set(BOARD_NAME W5500_EVB_PICO2)
set(BOARD_NAME W6100_EVB_PICO2)
# set(BOARD_NAME W6300_EVB_PICO2)


if(${BOARD_NAME} STREQUAL WIZnet_Ethernet_HAT)
    #set(PICO_PLATFORM rp2350) #If you are using WIZnet_ethernet_HAT with Pico2, uncomment it.
    set(WIZNET_CHIP W5100S)
    add_definitions(-D_WIZCHIP_=W5100S)
    add_definitions(-DDEVICE_BOARD_NAME=WIZnet_Ethernet_HAT)

elseif(${BOARD_NAME} STREQUAL W5100S_EVB_PICO)
    set(WIZNET_CHIP W5100S)
    add_definitions(-D_WIZCHIP_=W5100S)
    add_definitions(-DDEVICE_BOARD_NAME=W5100S_EVB_PICO)

elseif(${BOARD_NAME} STREQUAL W5500_EVB_PICO)
    set(WIZNET_CHIP W5500)
    add_definitions(-D_WIZCHIP_=W5500)
    add_definitions(-DDEVICE_BOARD_NAME=W5500_EVB_PICO)

elseif(${BOARD_NAME} STREQUAL W55RP20_EVB_PICO)
    set(WIZNET_CHIP W5500)
    add_definitions(-D_WIZCHIP_=W5500)
    add_definitions(-DDEVICE_BOARD_NAME=W55RP20_EVB_PICO)

elseif(${BOARD_NAME} STREQUAL W6100_EVB_PICO)
    set(WIZNET_CHIP W6100)
    add_definitions(-D_WIZCHIP_=W6100)
    add_definitions(-DDEVICE_BOARD_NAME=W6100_EVB_PICO)

elseif(${BOARD_NAME} STREQUAL W6300_EVB_PICO)
    set(WIZNET_CHIP W6300)
    add_definitions(-D_WIZCHIP_=W6300)
    add_definitions(-DDEVICE_BOARD_NAME=W6300_EVB_PICO)
    # Set QSPI MODE for W6300
    set(WIZCHIP_QSPI_MODE QSPI_QUAD_MODE)
    # set(WIZCHIP_QSPI_MODE QSPI_DUAL_MODE)
    # set(WIZCHIP_QSPI_MODE QSPI_SINGLE_MODE)

    add_definitions(-D_WIZCHIP_QSPI_MODE_=${WIZCHIP_QSPI_MODE})

elseif(${BOARD_NAME} STREQUAL W5100S_EVB_PICO2)
    set(WIZNET_CHIP W5100S)
    add_definitions(-D_WIZCHIP_=W5100S)
    add_definitions(-DDEVICE_BOARD_NAME=W5100S_EVB_PICO2)

elseif(${BOARD_NAME} STREQUAL W5500_EVB_PICO2)
    set(WIZNET_CHIP W5500)
    add_definitions(-D_WIZCHIP_=W5500)
    add_definitions(-DDEVICE_BOARD_NAME=W5500_EVB_PICO2)

elseif(${BOARD_NAME} STREQUAL W6100_EVB_PICO2)
    set(WIZNET_CHIP W6100)
    add_definitions(-D_WIZCHIP_=W6100)
    add_definitions(-DDEVICE_BOARD_NAME=W6100_EVB_PICO2)

elseif(${BOARD_NAME} STREQUAL W6300_EVB_PICO2)
    set(WIZNET_CHIP W6300)
    add_definitions(-D_WIZCHIP_=W6300)
    add_definitions(-DDEVICE_BOARD_NAME=W6300_EVB_PICO2)
    # Set QSPI MODE for W6300
    set(WIZCHIP_QSPI_MODE QSPI_QUAD_MODE)
    # set(WIZCHIP_QSPI_MODE QSPI_DUAL_MODE)
    # set(WIZCHIP_QSPI_MODE QSPI_SINGLE_MODE)

    add_definitions(-D_WIZCHIP_QSPI_MODE_=${WIZCHIP_QSPI_MODE})

else()
    message(FATAL_ERROR "BOARD_NAME is wrong = ${BOARD_NAME}")
endif()

message(STATUS "WIZNET_CHIP = ${WIZNET_CHIP}")

# Pull in WIZNET-PICO-C SDK version
include(wiznet_pico_c_sdk_version.cmake)

# Set the WIZNET_DIR and PORT_DIR
if(NOT DEFINED WIZNET_DIR)
    set(WIZNET_DIR ${CMAKE_SOURCE_DIR}/lib/WIZnet/ioLibrary_Driver)
    message(STATUS "WIZNET_DIR = ${WIZNET_DIR}")
endif()

if(NOT DEFINED PORT_DIR)
    set(PORT_DIR ${CMAKE_SOURCE_DIR}/lib/WIZnet/port)
    message(STATUS "PORT_DIR = ${PORT_DIR}")
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/WIZnet)
add_subdirectory(${PORT_DIR})

# ===========================================================

# ========================= FreeRTOS-Plus-CLI =========================
add_subdirectory(lib/FreeRTOS-Plus-CLI)
add_subdirectory(src/cli-task)

# ========================= Pico Flash Storage =========================
add_subdirectory(src/pico-flash-storage)

# ========================= Config =========================
add_subdirectory(src/config)

# ========================= HTTP Server =========================
add_subdirectory(src/http-server)

# ========================= Logger =========================
add_subdirectory(src/logger)

# ========================= Serial-to-TCP Bridge =========================
add_subdirectory(src/serial-to-tcp)

# ========================= Network Management =========================
add_subdirectory(src/network)

# ========================= RTOS Views Support =========================
add_subdirectory(src/rtos_views_support)

# ========================= MODBUS =========================
add_subdirectory(lib/nanoMODBUS)
add_subdirectory(src/modbus)

# ========================= Tag Database =========================
add_subdirectory(src/tag-database)

# Add executable. Default name is the project name, version 0.1

add_executable(pico-iiot-bridge src/main.c )

pico_set_program_name(pico-iiot-bridge "pico-iiot-bridge")
pico_set_program_version(pico-iiot-bridge "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico-iiot-bridge 1)
pico_enable_stdio_usb(pico-iiot-bridge 0)

# Add the standard library to the build
target_link_libraries(pico-iiot-bridge
        pico_stdlib
        pico_multicore
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
        freertos_config
        PORT_COMMON_FILES
        ETHERNET_FILES
        IOLIBRARY_FILES
        LOOPBACK_FILES
        cli-task
        pico_flash_storage
        system_config
        board_config
        logger
        http_server
        serial_to_tcp
        network_task
        rtos_views_support
        nanomodbus
        modbus
        tag-database)

# Add the standard include files to the build
target_include_directories(pico-iiot-bridge PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(pico-iiot-bridge)

