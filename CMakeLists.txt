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
set(CMAKE_PROJECT_NAME H750_TouchGFX)

set(UI_BACKEND "TOUCHGFX" CACHE STRING "UI backend: MINI or TOUCHGFX")
set_property(CACHE UI_BACKEND PROPERTY STRINGS MINI TOUCHGFX)
string(TOUPPER "${UI_BACKEND}" UI_BACKEND)
if(NOT UI_BACKEND STREQUAL "MINI" AND NOT UI_BACKEND STREQUAL "TOUCHGFX")
    message(FATAL_ERROR "UI_BACKEND must be MINI or TOUCHGFX")
endif()

option(ENABLE_CST816T_TOUCH "Enable CST816T touch controller on the configured I2C pins" OFF)

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

# Add touchgfx generated sources and libraries
if(UI_BACKEND STREQUAL "TOUCHGFX")
    add_subdirectory(cmake/touchgfx)
endif()

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/spi.c
    Core/Src/tim.c
    Core/Src/lcd_port.c
    Core/Src/qspi.c
    Core/Src/norflash.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_qspi.c
)

if(ENABLE_CST816T_TOUCH)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Src/i2c.c
        Core/Src/cst816t.c
    )
endif()

if(UI_BACKEND STREQUAL "TOUCHGFX")
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Src/ui_touchgfx.c
    )
else()
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Src/ui_mini.c
    )
endif()

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    UI_BACKEND_${UI_BACKEND}
    ENABLE_CST816T_TOUCH=$<BOOL:${ENABLE_CST816T_TOUCH}>
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

get_filename_component(TOOLCHAIN_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy
    PATHS "${TOOLCHAIN_BIN_DIR}"
    NO_DEFAULT_PATH
)
if(NOT ARM_OBJCOPY)
    set(ARM_OBJCOPY "${CMAKE_OBJCOPY}")
endif()

set(FLASH_ARTIFACT_BASE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}")
set(INTERNAL_FLASH_SECTIONS
    .isr_vector
    .text
    .rodata
    .ARM
    .preinit_array
    .init_array
    .fini_array
    .data
    TouchGFX_ScratchbufferB
    TouchGFX_ScratchbufferA
)

set(INTERNAL_FLASH_SECTION_ARGS)
foreach(section_name IN LISTS INTERNAL_FLASH_SECTIONS)
    list(APPEND INTERNAL_FLASH_SECTION_ARGS "--only-section=${section_name}")
endforeach()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND "${ARM_OBJCOPY}" -O ihex
            "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
            "${FLASH_ARTIFACT_BASE}.hex"
    COMMAND "${ARM_OBJCOPY}" -O ihex
            ${INTERNAL_FLASH_SECTION_ARGS}
            "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
            "${FLASH_ARTIFACT_BASE}_internal.hex"
    COMMENT "Generating full and internal STM32 flash HEX files"
)

if(UI_BACKEND STREQUAL "TOUCHGFX")
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND "${ARM_OBJCOPY}" -O ihex
                --only-section=TouchGFX_ExtFlash
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${FLASH_ARTIFACT_BASE}_extflash.hex"
        COMMAND "${ARM_OBJCOPY}" -O binary
                --only-section=TouchGFX_ExtFlash
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${FLASH_ARTIFACT_BASE}_extflash.bin"
        COMMENT "Generating TouchGFX external flash artifacts"
    )
endif()
