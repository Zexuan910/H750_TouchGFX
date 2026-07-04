/*
 * Linker script for the STM32H750 TouchGFX application executing from
 * memory-mapped W25Q64 QSPI NOR Flash at 0x90000000.
 */

ENTRY(Reset_Handler)

MEMORY
{
  DTCMRAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  RAM        (xrw) : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2     (xrw) : ORIGIN = 0x30000000, LENGTH = 288K
  RAM_D3     (xrw) : ORIGIN = 0x38000000, LENGTH = 64K
  ITCMRAM    (xrw) : ORIGIN = 0x00000000, LENGTH = 64K
  QSPI_FLASH (rx)  : ORIGIN = 0x90000000, LENGTH = 8M
}

_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);
_sstack = _estack - _Min_Stack_Size;
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >QSPI_FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >QSPI_FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >QSPI_FLASH

  TouchGFX_ExtFlash :
  {
    . = ALIGN(4);
    __touchgfx_extflash_start__ = .;
    KEEP(*(TouchGFXExtFlashSection*))
    KEEP(*(.TouchGFXExtFlashSection*))
    KEEP(*(ExtFlashSection*))
    KEEP(*(.ExtFlashSection*))
    KEEP(*(FontFlashSection*))
    KEEP(*(.FontFlashSection*))
    KEEP(*(FontSearchFlashSection*))
    KEEP(*(.FontSearchFlashSection*))
    KEEP(*(TextFlashSection*))
    KEEP(*(.TextFlashSection*))
    . = ALIGN(4);
    __touchgfx_extflash_end__ = .;
  } >QSPI_FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >QSPI_FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >QSPI_FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
    . = ALIGN(4);
  } >QSPI_FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
    . = ALIGN(4);
  } >QSPI_FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
    . = ALIGN(4);
  } >QSPI_FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
  } >DTCMRAM AT> QSPI_FLASH

  TouchGFX_ScratchbufferB :
  {
    . = ALIGN(4);
    *(TouchGFX_ScratchbufferB)
    *(.TouchGFX_ScratchbufferB)
    . = ALIGN(4);
  } >DTCMRAM AT> QSPI_FLASH

  TouchGFX_ScratchbufferA :
  {
    . = ALIGN(4);
    *(TouchGFX_ScratchbufferA)
    *(.TouchGFX_ScratchbufferA)
    . = ALIGN(4);
  } >DTCMRAM AT> QSPI_FLASH

  .tdata : ALIGN(4)
  {
    *(.tdata .tdata.* .gnu.linkonce.td.*)
    . = ALIGN(4);
    PROVIDE(__tdata_end = .);
    _edata = .;
  } >DTCMRAM AT> QSPI_FLASH

  .tbss (NOLOAD) : ALIGN(4)
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.tbss .tbss.*)
    . = ALIGN(4);
    PROVIDE(__tbss_end = .);
  } >DTCMRAM

  .bss (NOLOAD) : ALIGN(4)
  {
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
    PROVIDE(__bss_end = .);
  } >DTCMRAM

  TouchGFX_Framebuffer (NOLOAD) :
  {
    . = ALIGN(4);
    *(TouchGFX_Framebuffer)
    *(.TouchGFX_Framebuffer)
    . = ALIGN(4);
  } >RAM

  LCD_DMA_Buffer (NOLOAD) :
  {
    . = ALIGN(32);
    *(LCD_DMA_Buffer)
    *(.LCD_DMA_Buffer)
    . = ALIGN(32);
  } >RAM

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCMRAM

  /DISCARD/ :
  {
    *(.note.GNU-stack)
    *(.gnu_debuglink)
    *(.gnu.lto_*)
  }
}
