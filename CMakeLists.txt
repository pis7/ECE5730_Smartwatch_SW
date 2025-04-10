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
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(ECE5730_Smartwatch_SW C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(ECE5730_Smartwatch_SW)

add_compile_options(-Ofast)

# must match with executable name and source file names
target_sources(ECE5730_Smartwatch_SW PRIVATE 
    ECE5730_Smartwatch_SW.c
    font_ssh1106.c 
    ssh1106.c)

# must match with executable name
target_link_libraries(ECE5730_Smartwatch_SW PRIVATE 
    pico_stdlib 
    pico_divider 
    pico_multicore 
    pico_bootsel_via_double_reset 
    hardware_pio 
    hardware_dma 
    hardware_adc 
    hardware_irq 
    hardware_spi 
    hardware_clocks 
    hardware_pll 
    hardware_pwm 
    hardware_i2c)

# must match with executable name
pico_add_extra_outputs(ECE5730_Smartwatch_SW)
