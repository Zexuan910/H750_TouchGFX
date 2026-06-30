H750_TouchGFX download notes

This project uses STM32H750XBH6, whose internal Flash is only 128KB. The
TouchGFX images, fonts, and texts are intentionally placed in the linker
section TouchGFX_ExtFlash at address 0x90000000. Do not remove those assets to
make the firmware smaller if the full UI is required.

After a successful build, CMake generates these files in the build directory:

- H750_TouchGFX.hex
  Full Intel HEX. It contains both the internal Flash records at 0x08000000 and
  the TouchGFX external Flash records at 0x90000000.

- H750_TouchGFX_internal.hex
  Internal Flash only. This is enough for the MCU program, but not enough for
  the complete TouchGFX UI resources.

- H750_TouchGFX_extflash.hex
  TouchGFX external Flash only, addressed at 0x90000000.

- H750_TouchGFX_extflash.bin
  Raw TouchGFX external Flash payload. Program it at external Flash offset 0,
  which maps to MCU address 0x90000000 after QSPI memory-mapped mode is enabled.

If STM32CubeProgrammer or VS Code fails while downloading the full HEX/ELF, the
usual cause is that no matching external loader is selected for the board's
QSPI/NOR Flash. The internal 0x08000000 area can be programmed with the normal
STM32H750 algorithm, but 0x90000000 requires a board-specific external loader
or another trusted method to write the QSPI Flash.

The ALIENTEK H750 example project configures two Keil download algorithms:

- STM32H7x_128k.FLM at 0x08000000, size 0x020000
- ATK-DNH750_QSPI_W25Q64JV at 0x90000000, size 0x800000

That matches this project: internal code goes to 0x08000000, full TouchGFX
resources go to the W25Q64JV external Flash window at 0x90000000.

Current repository evidence:

- TouchGFX/application.config places image assets in ExtFlashSection.
- STM32H750XX_FLASH.ld maps TouchGFX_ExtFlash to EXTERNAL_FLASH at 0x90000000.
- The supplied ALIENTEK QSPI example maps W25Q64JV QSPI as PB2=CLK, PB6=NCS,
  PF8=IO0, PF9=IO1, PF7=IO2, PF6=IO3.
- This project now gives PB6 to QSPI_NCS by default and disables the CST816T
  PB6/PB7 touch path unless CMake is configured with ENABLE_CST816T_TOUCH=ON.
  Keep that option OFF until the touch controller has been moved to pins that
  do not conflict with the ALIENTEK W25Q64JV QSPI wiring.
- Startup initializes W25Q64JV, checks the JEDEC ID against supported Winbond
  IDs, enables QSPI memory-mapped mode, and only then starts TouchGFX.
