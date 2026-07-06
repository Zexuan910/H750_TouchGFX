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

set(APP_TARGET H750_TouchGFX_app)
set(BOOTLOADER_TARGET H750_TouchGFX_bootloader)

# Set the project name used by generated CubeMX/TouchGFX CMake fragments.
set(CMAKE_PROJECT_NAME ${APP_TARGET})

set(UI_BACKEND "TOUCHGFX" CACHE STRING "UI backend: MINI or TOUCHGFX")
set_property(CACHE UI_BACKEND PROPERTY STRINGS MINI TOUCHGFX)
string(TOUPPER "${UI_BACKEND}" UI_BACKEND)
if(NOT UI_BACKEND STREQUAL "MINI" AND NOT UI_BACKEND STREQUAL "TOUCHGFX")
    message(FATAL_ERROR "UI_BACKEND must be MINI or TOUCHGFX")
endif()

option(ENABLE_CST816T_TOUCH "Enable CST816T touch controller on the configured I2C pins" ON)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create the QSPI application executable.
add_executable(${APP_TARGET})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add touchgfx generated sources and libraries
if(UI_BACKEND STREQUAL "TOUCHGFX")
    add_subdirectory(cmake/touchgfx)
endif()

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${APP_TARGET} PRIVATE
    Core/Src/lcd_port.c
    Core/Src/max30102.c
    Core/Src/max30102_service.c
    Core/Src/imu_sensor.c
    Core/Src/walk_metrics.c
    Core/Src/usart.c
)

if(ENABLE_CST816T_TOUCH)
    target_sources(${APP_TARGET} PRIVATE
        Core/Src/cst816t.c
    )
endif()

if(UI_BACKEND STREQUAL "TOUCHGFX")
    target_sources(${APP_TARGET} PRIVATE
        Core/Src/ui_touchgfx.c
        Core/Src/ui_touchgfx_vsync.cpp
    )
else()
    target_sources(${APP_TARGET} PRIVATE
        Core/Src/ui_mini.c
    )
endif()

# Add include paths
target_include_directories(${APP_TARGET} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${APP_TARGET} PRIVATE
    UI_BACKEND_${UI_BACKEND}
    ENABLE_CST816T_TOUCH=$<BOOL:${ENABLE_CST816T_TOUCH}>
    APP_EXECUTES_FROM_QSPI=1
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${APP_TARGET}
    stm32cubemx

    # Add user defined libraries
)

add_executable(${BOOTLOADER_TARGET})
target_sources(${BOOTLOADER_TARGET} PRIVATE
    Core/Src/bootloader_main.c
    Core/Src/qspi.c
    Core/Src/norflash.c
    Core/Src/stm32h7xx_it.c
    Core/Src/syscalls.c
    Core/Src/sysmem.c
    startup_stm32h750xx.s
)
target_compile_definitions(${BOOTLOADER_TARGET} PRIVATE
    BOOTLOADER_BUILD=1
)
target_link_libraries(${BOOTLOADER_TARGET} PRIVATE
    stm32cubemx
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)

function(configure_stm32_firmware_target target linker_script)
    target_link_options(${target} PRIVATE
        "-T${CMAKE_SOURCE_DIR}/${linker_script}"
        "-Wl,-Map=${target}.map"
        "-Wl,--gc-sections"
        "-Wl,--print-memory-usage"
    )
    set_target_properties(${target} PROPERTIES
        ADDITIONAL_CLEAN_FILES "${target}.map"
    )
endfunction()

configure_stm32_firmware_target(${APP_TARGET} STM32H750XX_QSPI_APP.ld)
configure_stm32_firmware_target(${BOOTLOADER_TARGET} STM32H750XX_BOOTLOADER.ld)

get_filename_component(TOOLCHAIN_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy
    PATHS "${TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
if(NOT ARM_OBJCOPY)
    set(ARM_OBJCOPY "${CMAKE_OBJCOPY}")
endif()

function(add_firmware_artifacts target)
    set(FLASH_ARTIFACT_BASE "${CMAKE_CURRENT_BINARY_DIR}/${target}")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${ARM_OBJCOPY}" -O ihex
                "$<TARGET_FILE:${target}>"
                "${FLASH_ARTIFACT_BASE}.hex"
        COMMAND "${ARM_OBJCOPY}" -O binary
                "$<TARGET_FILE:${target}>"
                "${FLASH_ARTIFACT_BASE}.bin"
        COMMENT "Generating ${target} HEX and BIN files"
    )
endfunction()

add_firmware_artifacts(${APP_TARGET})
add_firmware_artifacts(${BOOTLOADER_TARGET})
