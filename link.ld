MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 512K
    RAM2 (xrw)      : ORIGIN = 0x24000000, LENGTH = 512K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    .ram2 :
    {
        . = ALIGN(4);
        *(.ram2*)
        . = ALIGN(4);
    } >RAM2
}
