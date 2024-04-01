# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set board as PICO W
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(sensing_unit C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    ADE9000API_RP2040.c
    bme280.c
    blocking.c
    tcp_server_new.c
    main.c
)

# Point to a custom linker script which reserves memory in flash
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap_custom.ld)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_i2c
)

# Link to wifi libraries
target_link_libraries(${PROJECT_NAME} 
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        pico_cyw43_arch_lwip_threadsafe_background
        )

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)   

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_set_program_name(${PROJECT_NAME} "SensingUnit")
pico_set_program_version(${PROJECT_NAME} "0.1")