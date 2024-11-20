/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                      2048Kbytes FLASH
**                      512Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
** All rights reserved.</center></h2>
**
** This software component is licensed by ST under BSD 3-Clause license,
** the "License"; You may not use this file except in compliance with the
** License. You may obtain a copy of the License at:
**                        opensource.org/licenses/BSD-3-Clause
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_AXI) + LENGTH(RAM_AXI); /* end of "RAM" Ram type memory */

_Min_Heap_Size = 0x400; /* required amount of heap  */
_Min_Stack_Size = 0x800; /* required amount of stack */

/* Memories definition */
MEMORY
{
  FLASH (rx)    			: ORIGIN = 0x08040000, LENGTH = (1024K - 256K)
  DTCM (rw)   				: ORIGIN = 0x20000000, LENGTH = 128K
  RAM_SHARED (rw) 			: ORIGIN = 0x24000000, LENGTH = 256
  RAM_SHARED_CACHED (rw) 	: ORIGIN = 0x24000000 + LENGTH(RAM_SHARED), LENGTH = 16K
  RAM_AXI (rwx)   			: ORIGIN = 0x24000000 + LENGTH(RAM_SHARED) + LENGTH(RAM_SHARED_CACHED), LENGTH = 512K - (LENGTH(RAM_SHARED) + LENGTH(RAM_SHARED_CACHED))
  RAM_D1 (r)      			: ORIGIN = 0x30000000, LENGTH = 128K
  RAM_D2 (rw)   			: ORIGIN = 0x30020000, LENGTH = 160K
  RAM_D3 (rw)    			: ORIGIN = 0x38000000, LENGTH = 64K
  ITCMRAM (rw)   			: ORIGIN = 0x00000000, LENGTH = 64K
}

/* Sections */
SECTIONS
{
  /* The startup code into "FLASH" Rom type memory */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data into "FLASH" Rom type memory */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data into "FLASH" Rom type memory */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { 
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH
  
  .ARM : {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH
  
  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH
  
  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH
  
  /* user sections... */
  .dtcm :
  {
    . = ALIGN(4);
    KEEP (*(.cisDataCpy));
    KEEP (*(.cisCals));  
    /* KEEP (*(.cisRGBCalibration)); */
    /* KEEP (*(.cisLedsCalibration)); */  
    . = ALIGN(4);
  } >DTCM AT> FLASH
  
  .axi :
  {
    . = ALIGN(4);
    KEEP (*(.cisData)); 
    . = ALIGN(4);
  } >RAM_AXI AT> FLASH
  
  .shared :
  {
    . = ALIGN(4);
    KEEP (*(.shared_var));  
    KEEP (*(.shared_config));
    . = ALIGN(4); 
  } >RAM_SHARED AT> FLASH

  .shared_cached :
  {
    . = ALIGN(4);
    KEEP (*(.rgbBuffers));
    KEEP (*(.imuData));    
    . = ALIGN(4);    
  } >RAM_SHARED_CACHED AT> FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections into "RAM_AXI" Ram type memory */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_AXI AT> FLASH

  /* Uninitialized data section into "RAM_AXI" Ram type memory */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_AXI
  
  _sram_axi_start = ORIGIN(RAM_SHARED);
  _sram_axi_end =  ORIGIN(RAM_SHARED) + LENGTH(RAM_SHARED) + LENGTH(RAM_SHARED_CACHED) + LENGTH(RAM_AXI);
  
  _sram_dtcm_start = ORIGIN(DTCM);
  _sram_dtcm_end = ORIGIN(DTCM) + LENGTH(DTCM);
  
  _sram_d2_start = ORIGIN(RAM_D2);
  _sram_d2_end = ORIGIN(RAM_D2) + LENGTH(RAM_D2);

  /* User_heap_stack section, used to check that there is enough "RAM_AXI" Ram  type memory left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_AXI
  
    .lwip_sec (NOLOAD) :
  {
    . = ABSOLUTE(0x30040000);
    *(.RxDecripSection) 
    
    . = ABSOLUTE(0x30040100);
    *(.TxDecripSection)
    
    . = ABSOLUTE(0x30040200);
    *(.Rx_PoolSection)  
  } >RAM_D2

  /* Remove information from the compiler libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
