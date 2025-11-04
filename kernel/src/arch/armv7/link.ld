SECTIONS {
  . = 0x4f000000;

  .text : {
    *(.text*)
  }
    
  .data : {
    *(.data*)
  }
}
