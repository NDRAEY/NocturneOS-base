ENTRY (__pre_init)

SECTIONS
{
    . = 0x00100000;

    KERNEL_BASE_pos = .;
    
    .text ALIGN (0x1000) : {
        KEEP(*(.mboot))
        *(.mboot)

        CODE_start = .;

        *(.text*)

        CODE_end = .;
    }

    .data ALIGN (0x1000) :
    {
        DATA_start = .;

        cpu_vendor_start = .;
        *(.cpu_vendor_data*)
        cpu_vendor_end = .;
        
        . = ALIGN(8);

        *(.data*)

        DATA_end = .;
    }

    .rodata ALIGN (0x8000) :
    {
        RODATA_start = .;

        *(.rodata*)

        RODATA_end = .;
    }

    .bss ALIGN (0x1000) :
    {
        *(COMMON)

        BSS_start = .;

        *(.bss*)

        BSS_end = .;
    }

    .debug_symbols ALIGN(0x1000) : {
        *(.debug_symbols*)
    }

    .bootstrap_tables ALIGN(0x1000) : {
        *(.bootstrap_table*)
    }

    /DISCARD/ : {
        *(.note.gnu.build-id)
        *(.eh_frame)
    }
    
    KERNEL_END_pos = .;

}
