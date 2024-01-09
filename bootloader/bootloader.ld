OUTPUT_ARCH( "riscv" )

/* Task: ensure the correct starting function. */
ENTRY( _entry )

SECTIONS
{
 /* Task lay out the sections as mentioned in the handout */

   
  /* Task: ensure the correct entry address based on where -kernel in QEMU jumps. */
  . = 0x80000000;
  
  .text : {
   *(.text)
   PROVIDE(etext = 0x80008000);
  }

  .data : {
   *(.data)
   PROVIDE(edata = 0x80008044);
  }
  
  .rodata : {
   *(.rodata)
   PROVIDE(erodata = 0x80008990);
  }

  .bss : {
   *(.bss)
   PROVIDE(ebss = 0x80021df0);
  }

  
  
  

  

  PROVIDE(end = 0x8005be1d);
}
