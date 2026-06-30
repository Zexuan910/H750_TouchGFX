set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# Some default GCC settings
set(TOOLCHAIN_PREFIX                arm-none-eabi-)

file(GLOB STM32CUBE_GCC_BIN_DIRS
    "$ENV{LOCALAPPDATA}/stm32cube/bundles/gnu-tools-for-stm32/*/bin"
)
list(SORT STM32CUBE_GCC_BIN_DIRS)
list(REVERSE STM32CUBE_GCC_BIN_DIRS)

find_program(ARM_NONE_EABI_GCC ${TOOLCHAIN_PREFIX}gcc
    PATHS ${STM32CUBE_GCC_BIN_DIRS}
)
find_program(ARM_NONE_EABI_GXX ${TOOLCHAIN_PREFIX}g++
    PATHS ${STM32CUBE_GCC_BIN_DIRS}
)
find_program(ARM_NONE_EABI_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy
    PATHS ${STM32CUBE_GCC_BIN_DIRS}
)
find_program(ARM_NONE_EABI_SIZE ${TOOLCHAIN_PREFIX}size
    PATHS ${STM32CUBE_GCC_BIN_DIRS}
)

if(NOT ARM_NONE_EABI_GCC OR NOT ARM_NONE_EABI_GXX)
    message(FATAL_ERROR "arm-none-eabi GCC toolchain not found. Install STM32Cube bundled GNU tools or add arm-none-eabi-* to PATH.")
endif()

set(CMAKE_C_COMPILER                ${ARM_NONE_EABI_GCC})
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER              ${ARM_NONE_EABI_GXX})
set(CMAKE_LINKER                    ${ARM_NONE_EABI_GXX})
set(CMAKE_OBJCOPY                   ${ARM_NONE_EABI_OBJCOPY})
set(CMAKE_SIZE                      ${ARM_NONE_EABI_SIZE})

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections -fstack-usage")

# The cyclomatic-complexity parameter must be defined for the Cyclomatic complexity feature in STM32CubeIDE to work.
# However, most GCC toolchains do not support this option, which causes a compilation error; for this reason, the feature is disabled by default.
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcyclomatic-complexity")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32H750XX_FLASH.ld\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
set(TOOLCHAIN_LINK_LIBRARIES "m")
