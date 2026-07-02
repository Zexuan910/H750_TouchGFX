# H750 TouchGFX 烧录闭环状态

更新时间：2026-07-02 12:11:36 +08:00 起第一轮手动执行。

原始目标：让当前工程成功烧录到 STM32H750 内部 Flash 和外部 QSPI Flash，并基于证据确认启动链路可运行。

## 第一轮证据

- 工作区状态：`git status --short` 显示未跟踪 `AGENTS.MD`，本轮未修改用户已有源码。
- `cmake` 不在当前 PATH 中；已使用 STM32Cube 捆绑 CMake：`%LOCALAPPDATA%\stm32cube\bundles\cmake\4.3.1+st.1\bin\cmake.exe`。
- Release 配置成功，构建目录：`build/Release`。
- Release 构建成功，输出为 `ninja: no work to do`。
- `tools/verify_qspi_bootloader_layout.ps1 -Config Release` 通过。
- `tools/verify_qspi_loader.ps1 -LoaderPath tools/loaders/ATK-DNH750_QSPI_W25Q64JV.stldr` 通过。
- `tools/program_qspi_split.ps1 -Config Release -DryRun` 通过，顺序正确：先 app 写 `0x90000000` QSPI，再 bootloader 写 `0x08000000` 内部 Flash 并复位。

## 实际烧录结果

实际执行：

```powershell
powershell -ExecutionPolicy Bypass -File tools/program_qspi_split.ps1 -Config Release -LoaderPath tools/loaders/ATK-DNH750_QSPI_W25Q64JV.stldr
```

CubeProgrammer 能连接目标：

- STM32CubeProgrammer v2.22.0
- ST-LINK SN：6202040222005A504E413836
- ST-LINK FW：V2J47S7
- Voltage：3.28V
- Connect mode：Under Reset
- Device ID：0x450
- Device：STM32H7xx
- NVM size：128 KBytes

卡点：写 app 到 QSPI 时，日志停在：

```text
Erasing memory corresponding to segment 0:
Erasing external memory sectors [0 39]
```

外层命令 300 秒超时，没有完成 app 写入、校验、bootloader 写入或复位运行。因此原始目标尚未达成。

随后尝试读取 loader 调试区：

```powershell
STM32_Programmer_CLI.exe -c port=SWD -r32 0x2001FFE0 8
STM32_Programmer_CLI.exe -c port=SWD mode=UR -r32 0x2001FFE0 8
```

两次均返回：

```text
ST-LINK error (DEV_USB_COMM_ERR)
```

且 `Get-Process STM32_Programmer_CLI -ErrorAction SilentlyContinue` 未发现残留进程。

## 当前判断

软件前置条件已通过，但实际目标未达成。当前阻塞点不是普通构建失败，而是实际 QSPI 擦除阶段长时间未完成，随后 ST-LINK USB 通信错误。

下一轮优先事项：

1. 用户需要先物理断电重连开发板和/或重新插拔 ST-LINK USB。
2. 重新连接后先验证 ST-LINK 通信，不要立即重复完整烧录。
3. 若能连接，优先读取 `0x2001FFE0` loader 调试区，判断上次 loader 最后阶段；若 SRAM 已丢失，则重新执行外部 loader 最小化测试。
4. 若继续卡在外部 Flash 擦除，重点排查 loader 的 `norflash_wait_busy()` 是否因 DWT/HAL tick 或 Flash BUSY 状态无法退出，以及 QSPI 引脚/Flash 型号/供电是否符合 W25Q64JV 假设。
