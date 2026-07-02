# AGENTS.MD

本文件记录对当前仓库的项目理解与协作约定，供后续在本目录内工作的 agent 参考。更新日期：2026-07-02。

## 协作约定

- 使用中文沟通与记录。教学或解释时，可以适当扩展成稳定工作流、排查顺序、验证习惯等方法论。
- 不要瞎猜；硬件接线、烧录状态、工具版本、板卡现象等不确定时，先说明不确定点并向用户确认。
- 开始动手前先看当前文件与 `git status`，不要覆盖用户已有改动。本仓库当前已有较多未提交改动，后续任务应只改与需求相关的文件。
- 遇到 CubeMX、TouchGFX Designer、生成代码或硬件相关问题时，优先用已有工程结构和脚本验证，不要只凭经验改。

## 项目概览

- 项目名：`H750_TouchGFX`。
- MCU：`STM32H750XBH6`，`TFBGA240`，内部 Flash 只有 128KB。
- 工程形态：STM32CubeMX + TouchGFX 4.26.1 + CMake/Ninja + arm-none-eabi GCC。
- 固件被拆成两个目标：
  - `H750_TouchGFX_bootloader`：运行在内部 Flash `0x08000000`，负责初始化 QSPI NOR Flash、进入 memory-mapped 模式、校验 app 向量表、设置 `SCB->VTOR = 0x90000000`，然后跳到 app。
  - `H750_TouchGFX_app`：链接并运行在外部 QSPI NOR Flash `0x90000000`，TouchGFX 代码、常量、图片、字体和文本资源都在 8MB QSPI 窗口内。
- 主要目录：
  - `Core/`：CubeMX 生成和用户补充的 MCU 初始化、LCD/QSPI/NOR Flash/触摸相关 C 代码。
  - `TouchGFX/`：TouchGFX Designer 工程、GUI 源码、生成资源、target 适配层。
  - `Drivers/`：STM32H7 HAL/CMSIS。
  - `Middlewares/ST/touchgfx/`：TouchGFX 框架和库。
  - `cmake/`：CubeMX 与 TouchGFX 的 CMake 片段。
  - `tools/`：QSPI 外部 loader、烧录和布局验证脚本。

## 构建方式

- CMake preset：`Debug`、`Release`，构建目录为 `build/<Config>`。
- 常用命令：

```powershell
cmake --preset Release
cmake --build --preset Release
```

- 成功构建后通常生成：
  - `build/Release/H750_TouchGFX_bootloader.elf/.hex/.bin/.map`
  - `build/Release/H750_TouchGFX_app.elf/.hex/.bin/.map`
- `.clangd` 使用 `build/Release/compile_commands.json`。
- 顶层 `CMakeLists.txt` 中默认 `UI_BACKEND=TOUCHGFX`，另有 `MINI` 后端源文件；但当前 `Core/Src/main.c` 直接调用 `MX_TouchGFX_Init()` 和 `MX_TouchGFX_Process()`。
- `ENABLE_CST816T_TOUCH` 默认是 `OFF`，不要随手打开，原因见“触摸状态”。

## 链接与内存布局

- `STM32H750XX_BOOTLOADER.ld`：bootloader 放内部 Flash `0x08000000`，长度 128KB。
- `STM32H750XX_QSPI_APP.ld`：app 放 QSPI Flash `0x90000000`，长度 8MB。
- app 的 `TouchGFX_Framebuffer` 放 AXI SRAM `0x24000000`。
- app 和 bootloader 都配置了 QSPI 窗口的 MPU 区域，使 `0x90000000` 的 8MB 区域可执行且 cacheable。
- app 目标定义 `APP_EXECUTES_FROM_QSPI=1`。在这个模式下，app 不应重新初始化、abort、擦除或重配 QSPI；QSPI 初始化和 memory-mapped 模式由 bootloader 负责。

## QSPI 与烧录流程

- 外部 Flash 目标是 W25Q 系列，项目围绕 `W25Q64JV` 的 8MB 容量工作。
- QSPI 引脚：
  - `PB2`：QSPI_CLK
  - `PB6`：QSPI_NCS
  - `PF8`：QSPI_BK1_IO0
  - `PF9`：QSPI_BK1_IO1
  - `PF7`：QSPI_BK1_IO2
  - `PF6`：QSPI_BK1_IO3
- 正确烧录顺序：
  1. 先用 STM32CubeProgrammer 外部 loader 把 `H750_TouchGFX_app` 写到 QSPI `0x90000000`。
  2. 再把 `H750_TouchGFX_bootloader` 写到内部 Flash `0x08000000` 并复位运行。
- 辅助脚本：

```powershell
powershell -ExecutionPolicy Bypass -File tools/program_qspi_split.ps1 -Config Release -DryRun
powershell -ExecutionPolicy Bypass -File tools/program_qspi_split.ps1 -Config Release -LoaderPath tools/loaders/ATK-DNH750_QSPI_W25Q64JV.stldr
```

- `tools/loaders/ATK-DNH750_QSPI_W25Q64JV.stldr` 是 STM32CubeProgrammer 需要的外部 loader。`.FLM` 是 Keil/MDK flash algorithm，不能直接改名当 `.stldr` 用。
- 如果 bootloader 已经运行过，可能把 QSPI 留在 memory-mapped 模式，导致外部 loader 下载 app 失败；此时使用 Under Reset 连接，必要时断电重连。
- `.vscode/launch.json` 当前只加载 `build/Release/H750_TouchGFX_bootloader.elf`。不要让 ST-LINK GDB 直接 `load` app ELF，因为 app 地址在 `0x90000000`，需要外部 loader。

## LCD 显示链路

- 显示屏按 `SCREEN_COLOR` 的 1.69 寸 SPI 屏配置对齐，尺寸为 `240x280`，`LCD_PORT_Y_OFFSET=20`，RGB565。
- LCD 引脚：
  - `PB5`：LCD_SDA / SPI1_MOSI
  - `PG11`：LCD_SCK / SPI1_SCK
  - `PG9`：SPI1_MISO
  - `PJ6`：LCD_CS
  - `PJ7`：LCD_RST
  - `PJ8`：LCD_DC
  - `PA8`：LCD_BL / TIM1_CH1 PWM
- 关键文件：
  - `Core/Inc/lcd_port.h`：屏幕尺寸、偏移、RGB565 颜色定义和接口。
  - `Core/Src/lcd_port.c`：LCD 初始化、SPI 写窗口、背光 PWM、矩形/像素/文本绘制。
  - `TouchGFX/target/TouchGFXHAL.cpp`：`flushFrameBuffer(rect)` 将 TouchGFX framebuffer 的脏矩形逐行写入 SPI LCD。
- SPI1 使用 `SPI_BAUDRATEPRESCALER_16`；TIM1 CH1 控制背光。

## TouchGFX 状态

- TouchGFX Designer 版本：4.26.1。
- 显示方向：Portrait，画布 `240x280`，16bpp。
- 图片配置：RGB565/ARGB8888，资源 section 是 `ExtFlashSection`。
- 文本语言：`GB`；默认字体 `simsunb.ttf` 20px，另有 `verdana.ttf` 40px/10px。
- 启动屏幕：`screen1`。
- 当前 GUI 逻辑：
  - `screen1`：点击上方区域循环显示红/绿/蓝、英文/中文文本、图片 A/B 等 demo 状态。
  - `Screen2`：上下滑动切换 5 页矩形内容；左滑进入 `Screen3`。
  - `Screen3`：右滑返回 `Screen2`。
- `cmake/touchgfx/CMakeLists.txt` 是 TouchGFX Designer 自动生成文件，手工改动容易被重新 Generate Code 覆盖。

## 触摸状态

- 仓库里有 CST816T 驱动文件：
  - `Core/Inc/i2c.h`
  - `Core/Src/i2c.c`
  - `Core/Inc/cst816t.h`
  - `Core/Src/cst816t.c`
- CST816T 代码默认地址是 7-bit `0x15`，坐标方向可在 `Core/Inc/cst816t.h` 中通过 `CST816T_SWAP_XY`、`CST816T_MIRROR_X`、`CST816T_MIRROR_Y` 调整。
- 但当前 TouchGFX 的 `TouchGFX/target/STM32TouchController.cpp` 仍然 `return false`，也就是说 TouchGFX 真机触摸采样尚未接通。
- 更关键的是：`Core/Src/i2c.c` 写的是 `PB6/PB7` 作为 I2C1 SCL/SDA，而当前 QSPI 使用 `PB6` 作为 `QSPI_NCS`。在 QSPI app 架构下，`PB6` 不能同时作为触摸 SCL。除非硬件触摸线已经改到不冲突的引脚，否则保持 `ENABLE_CST816T_TOUCH=OFF`。

## 生成代码与修改边界

- CubeMX 可能改写 `Core/Src/main.c`、`gpio.c`、`spi.c`、`tim.c`、`quadspi.c`、`cmake/stm32cubemx/CMakeLists.txt` 等文件。修改这些文件时尽量放在 USER CODE 区，或记录清楚需要在重新生成后手工恢复的点。
- TouchGFX Designer 可能改写 `TouchGFX/generated/`、`TouchGFX/generated/gui_generated/`、`cmake/touchgfx/CMakeLists.txt` 等生成文件。业务逻辑优先放在 `TouchGFX/gui/src/` 和 `TouchGFX/gui/include/`。
- `Drivers/`、`Middlewares/` 是第三方/厂商代码，除非明确需要，不要做大范围手工改动。
- Windows 下已有一些中文文件名的乱码重复文件；引用说明时优先使用正常中文文件名，如 `README_TOUCH_触摸补充说明.txt`、`修改说明_按SCREEN_COLOR为准.txt`。

## 验证命令

- 构建 Release：

```powershell
cmake --preset Release
cmake --build --preset Release
```

- 验证 bootloader/app 布局：

```powershell
powershell -ExecutionPolicy Bypass -File tools/verify_qspi_bootloader_layout.ps1 -Config Release
```

- 构建外部 loader：

```powershell
powershell -ExecutionPolicy Bypass -File tools/build_qspi_loader.ps1 -Config Release
```

- 验证外部 loader：

```powershell
powershell -ExecutionPolicy Bypass -File tools/verify_qspi_loader.ps1 -LoaderPath tools/loaders/ATK-DNH750_QSPI_W25Q64JV.stldr
```

- 打印烧录命令但不实际下载：

```powershell
powershell -ExecutionPolicy Bypass -File tools/program_qspi_split.ps1 -Config Release -DryRun
```
