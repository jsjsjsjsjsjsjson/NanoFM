cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME NanoFM)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # FreeRTOS
    FreeRTOS-Kernel/tasks.c
    FreeRTOS-Kernel/queue.c
    FreeRTOS-Kernel/list.c
    FreeRTOS-Kernel/timers.c
    FreeRTOS-Kernel/event_groups.c
    FreeRTOS-Kernel/stream_buffer.c

    FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c
    FreeRTOS-Kernel/portable/MemMang/heap_4.c

    # TinyUSB
    tinyusb/src/tusb.c
    tinyusb/src/common/tusb_fifo.c

    tinyusb/src/device/usbd.c
    tinyusb/src/device/usbd_control.c

    tinyusb/src/class/cdc/cdc_device.c
    tinyusb/src/class/midi/midi_device.c

    tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c

    ${CMAKE_SOURCE_DIR}/Src/usb_descriptors.c

    # App
    app_main/src/app_main.cpp
    app_main/src/synth_core.cpp

    # Other
    Src/cdc_stdio.c
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wdouble-promotion

    $<$<CONFIG:Debug>:-Ofast>
    $<$<CONFIG:Debug>:-g3>

    $<$<CONFIG:Release>:-Ofast>
    $<$<CONFIG:Release>:-g0>
    $<$<CONFIG:Release>:-DNDEBUG>
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    FreeRTOS-Kernel/include
    FreeRTOS-Kernel/portable/GCC/ARM_CM3
    tinyusb/hw

    app_main/inc
)

target_include_directories(${CMAKE_PROJECT_NAME}
    SYSTEM PRIVATE
        tinyusb/src
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
