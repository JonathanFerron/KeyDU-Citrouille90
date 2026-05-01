/*
 * KeyDU.BL — Bootloader linker script
 * AVR64DU32: BOOT region 0x0000–0x1FFF (8KB)
 *
 * Note: AVR byte addresses in linker scripts; avr-gcc -mmcu=avr64du32
 * automatically provides __DATA_REGION_ORIGIN__ etc., but we override
 * flash origin explicitly for the boot split.
 */

MEMORY
{
    flash  (rx)  : ORIGIN = 0x0000, LENGTH = 8K
    sram   (rw!x): ORIGIN = 0x802000, LENGTH = 8K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))       /* Interrupt vector table must be first */
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
