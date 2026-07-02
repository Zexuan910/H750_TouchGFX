ENTRY(Init)

MEMORY
{
  DEVINFO (r)  : ORIGIN = 0x00000000, LENGTH = 0x00000100
  RAM     (rwx): ORIGIN = 0x20000004, LENGTH = 0x0001FFFC
}

PHDRS
{
  devinfo PT_LOAD FLAGS(4);
  ram PT_LOAD FLAGS(7);
}

SECTIONS
{
  .Dev_Info 0x00000000 :
  {
    KEEP(*(.Dev_Info))
  } > DEVINFO :devinfo

  .text 0x20000004 :
  {
    . = ALIGN(4);
    *(.text*)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    . = ALIGN(4);
  } > RAM :ram

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > RAM :ram

  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > RAM :ram

  .data :
  {
    . = ALIGN(4);
    *(.data*)
    . = ALIGN(4);
  } > RAM :ram

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
  } > RAM :ram

  /DISCARD/ :
  {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
  }
}
