cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

project(ATK_DNH750_QSPI_W25Q64JV_LOADER C)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(LOADER_TARGET ATK-DNH750_QSPI_W25Q64JV)

add_executable(${LOADER_TARGET}
    Src/loader_api.c
    Src/loader_hal_stubs.c
    ${REPO_ROOT}/Core/Src/qspi.c
    ${REPO_ROOT}/Core/Src/norflash.c
    ${REPO_ROOT}/Core/Src/system_stm32h7xx.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_qspi.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.c
)

target_include_directories(${LOADER_TARGET} PRIVATE
    Inc
    ${REPO_ROOT}/Core/Inc
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${REPO_ROOT}/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${REPO_ROOT}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${REPO_ROOT}/Drivers/CMSIS/Include
)

target_compile_definitions(${LOADER_TARGET} PRIVATE
    STM32H750xx
    USE_HAL_DRIVER
    USE_PWR_LDO_SUPPLY
    EXTERNAL_LOADER_BUILD=1
)

target_compile_options(${LOADER_TARGET} PRIVATE
    -Os
    -Wall
    -Wextra
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
    -fno-common
)

target_link_options(${LOADER_TARGET} PRIVATE
    "-T${CMAKE_CURRENT_LIST_DIR}/STM32H750XX_QSPI_LOADER.ld"
    "-nostartfiles"
    "-Wl,--gc-sections"
    "-Wl,--undefined=StorageInfo"
    "-Wl,--undefined=Init"
    "-Wl,--undefined=Read"
    "-Wl,--undefined=Write"
    "-Wl,--undefined=SectorErase"
    "-Wl,--undefined=MassErase"
    "-Wl,--undefined=Verify"
    "-Wl,--undefined=CheckSum"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${LOADER_TARGET}.map"
)

set_target_properties(${LOADER_TARGET} PROPERTIES
    OUTPUT_NAME "${LOADER_TARGET}"
    SUFFIX ".elf"
)
