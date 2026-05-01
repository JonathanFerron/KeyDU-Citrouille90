/*
 * KeyDU.App — Application linker script
 * AVR64DU32: APPCODE region 0x2000–0xFFFF (56KB)
 */

MEMORY
{
    flash  (rx)  : ORIGIN = 0x2000, LENGTH = 56K
    sram   (rw!x): ORIGIN = 0x802000, LENGTH = 8K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))       /* App has its own vector table at 0x2000 */
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } > flash

    .data : AT (ADDR(.text) + SIZEOF(.text))
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
}
