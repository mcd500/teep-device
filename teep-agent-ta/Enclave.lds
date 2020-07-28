OUTPUT_ARCH( "riscv" )

SECTIONS 
{
  . = 0x00001000;
  .text : {
    *(.text._start)
    *(.text)
  }
  . = ALIGN(0x1000);
  .rodata : 
  { 
    *(.rdata)
    *(.rodata)
  }
  . = ALIGN(8);
  .init_array : {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array ))
    PROVIDE_HIDDEN (__init_array_end = .);
  }
  .fini_array : {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array ))
    PROVIDE_HIDDEN (__fini_array_end = .);
  }
  .data : { *(.data) }
  .bss : { *(.bss) }
  .debug : { *(.debug) }
  
  . = ALIGN(0x1000);
  .malloc_region : {
    __malloc_start = .;
    . = . + 4096*256;
    __malloc_end = .;
  }
  . = ALIGN(0x1000);
  .perf_region : {
    __perf_start = .;
    . = . + 4096*128;
    __perf_zone_stop = .;
  }

  _end = .;
}
