QSPI external loader files

Put the STM32CubeProgrammer external loader here when available:

  ATK-DNH750_QSPI_W25Q64JV.stldr

The project may also keep the vendor Keil/MDK flash algorithm here for
reference:

  ATK-DNH750_QSPI_W25Q64JV.FLM

Do not rename the .FLM file to .stldr. STM32CubeProgrammer expects a .stldr
external loader for the -el argument; Keil .FLM files use a different flash
algorithm interface.
