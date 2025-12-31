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

# Set the FreeRTOS Kernel path to the local FreeRTOS-Kernel folder
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)

# Add FreeRTOS configuration (must be before FreeRTOS kernel import)
add_subdirectory(lib/FreeRTOS-config)
# Pull in FreeRTOS Kernel
include(FreeRTOS_Kernel_import.cmake)

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
        freertos_config)

# Add the standard include files to the build
target_include_directories(pico-iiot-bridge PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(pico-iiot-bridge)

