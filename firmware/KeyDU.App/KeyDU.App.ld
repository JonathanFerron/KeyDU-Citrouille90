/*
 * KeyDU.App — Application linker script
 * AVR64DU32: APPCODE region 0x2000–0xFFFF (56KB)
 */

MEMORY
{
    flash  (rx)  : ORIGIN = 0x2000, LENGTH = 56K
    sram   (rw!x): ORIGIN = 0x806000, LENGTH = 8K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(2);
        __text_end = .;
    } > flash

    .data : AT (__text_end)
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    } > sram

    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } > sram

    __data_load_start = LOADADDR(.data);

    /DISCARD/ :
    {
        *flmap_init*(.init3)
    }
}
