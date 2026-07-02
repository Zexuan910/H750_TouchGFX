H750_TouchGFX bootloader + QSPI app download notes

This project uses STM32H750XBH6, whose internal Flash is only 128KB. The
firmware is split into two images:

- H750_TouchGFX_bootloader
  Runs from internal Flash at 0x08000000. It initializes the clock tree,
  initializes the W25Q64JV QSPI NOR Flash, enables QSPI memory-mapped mode,
  validates the application vector table, relocates VTOR to 0x90000000, and
  jumps to the QSPI application reset handler.

- H750_TouchGFX_app
  Runs from external QSPI NOR Flash at 0x90000000. Its vector table, code,
  read-only data, and TouchGFX image/font/text resources are all linked into
  the 8MB QSPI window. The TouchGFX framebuffer remains in AXI SRAM at
  0x24000000.

Generated artifacts after a successful build:

- build/<Config>/H750_TouchGFX_bootloader.elf
- build/<Config>/H750_TouchGFX_bootloader.hex
- build/<Config>/H750_TouchGFX_bootloader.bin
- build/<Config>/H750_TouchGFX_app.elf
- build/<Config>/H750_TouchGFX_app.hex
- build/<Config>/H750_TouchGFX_app.bin

Download sequence:

1. Program the app to QSPI NOR Flash with the board-specific external loader.
   Do this before running the bootloader. If the bootloader is already running,
   it can leave QSPI in memory-mapped mode and the external loader may fail
   with "failed to download Segment[0]".

   STM32_Programmer_CLI -c port=SWD mode=UR -el ATK-DNH750_QSPI_W25Q64JV.stldr -w build/Release/H750_TouchGFX_app.hex -v

   Or with the raw binary:

   STM32_Programmer_CLI -c port=SWD mode=UR -el ATK-DNH750_QSPI_W25Q64JV.stldr -w build/Release/H750_TouchGFX_app.bin 0x90000000 -v

2. Program the bootloader to internal Flash, then reset into it:

   STM32_Programmer_CLI -c port=SWD mode=UR -w build/Release/H750_TouchGFX_bootloader.hex -v -rst

STM32CubeProgrammer GUI notes:

- Prefer "Under Reset" for both downloads.
- For the QSPI app step, select `ATK-DNH750_QSPI_W25Q64JV.stldr`, program
  `H750_TouchGFX_app.hex`, and do not run at `0x90000000`.
- Program `H750_TouchGFX_bootloader.hex` last. Only after this final step should
  the board reset or run from `0x08000000`.
- If you already ran the bootloader before programming the app, power-cycle the
  board or reconnect under reset before retrying the QSPI app download.

Helper script:

   powershell -ExecutionPolicy Bypass -File tools/program_qspi_split.ps1 -Config Release -LoaderPath path\to\ATK-DNH750_QSPI_W25Q64JV.stldr

   To print the exact commands without flashing hardware:

   powershell -ExecutionPolicy Bypass -File tools/program_qspi_split.ps1 -Config Release -DryRun

VS Code debug:

- The checked-in `.vscode/launch.json` intentionally loads only
  `build/Release/H750_TouchGFX_bootloader.elf`.
- Do not use VS Code/ST-LINK GDB `load` on `H750_TouchGFX_app.elf` or the
  stale single-image `H750_TouchGFX.elf`, because both contain QSPI addresses
  at 0x90000000 that require an external loader.
- After the app is programmed with the external loader, debug the bootloader
  path or attach/load symbols without re-downloading the QSPI app.

Important:

- The app cannot be programmed to 0x90000000 with only the normal internal
  STM32H750 Flash algorithm. Use the ALIENTEK W25Q64JV external loader
  (`ATK-DNH750_QSPI_W25Q64JV`) or an equivalent board-specific loader.
- Place a matching STM32CubeProgrammer `.stldr` under `tools/loaders/` or pass
  it with `-LoaderPath`. A Keil/MDK `.FLM` may be kept there as reference, but
  it cannot be used directly with `STM32_Programmer_CLI -el`.
- The app must not reinitialize, abort, erase, or reconfigure QSPI while it is
  executing from QSPI. The bootloader owns QSPI setup and memory-mapped mode.
- `PB6` remains `QSPI_NCS`. Keep `ENABLE_CST816T_TOUCH=OFF` unless the touch
  controller wiring has been moved away from PB6/PB7.
- NAND and SDRAM are not used by this split. NAND should be reserved for future
  file storage or OTA packages, not direct code execution.

Repository evidence:

- QSPI wiring: PB2=CLK, PB6=NCS, PF8=IO0, PF9=IO1, PF7=IO2, PF6=IO3.
- Bootloader linker script: `STM32H750XX_BOOTLOADER.ld`.
- QSPI app linker script: `STM32H750XX_QSPI_APP.ld`.
- Layout verification script: `tools/verify_qspi_bootloader_layout.ps1`.
