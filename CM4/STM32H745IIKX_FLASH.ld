/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**			      		2048Kbytes FLASH
**                		512Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2019 STMicroelectronics.
** All rights reserved.
**
** This software component is licensed by ST under BSD 3-Clause license,
** the "License"; You may not use this file except in compliance with the
** License. You may obtain a copy of the License at:
**                        opensource.org/licenses/BSD-3-Clause
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_D1A) + LENGTH(RAM_D1A); /* 0x10048000;    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x400;      /* required amount of heap  */
_Min_Stack_Size = 0x800; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
  /* Doit rester identique a FW_CM4_START_ADDR / FW_CM4_MAX_SIZE de
     Common/Inc/boot_config.h : c'est cette taille que le bootloader
     sauvegarde et restaure. Une image qui deborderait au-dela ne serait
     que partiellement rendue par un rollback. 640K = secteurs 2 a 6 du
     bank 1, le secteur 7 restant reserve. */
  FLASH (rx)      			: ORIGIN = 0x08040000, LENGTH = 640K
  RAM_SHARED (rw) 			: ORIGIN = 0x24000000, LENGTH = 2K
  RAM_SHARED_CACHED (rw) 	: ORIGIN = 0x24000000 + LENGTH(RAM_SHARED), LENGTH = 2K
  RAM_D1A (rw)    			: ORIGIN = 0x10000000, LENGTH = 40K
  RAM_D4 (xrw)    			: ORIGIN = 0x38000000, LENGTH = 64K
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
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

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH
  
  /* user sections... */
  /* Pas de "AT> FLASH" sur les sections NOLOAD : elles ne contiennent aucune
     donnee a charger, mais un AT> leur donne quand meme une LMA en flash,
     identique a celle de .data. STM32CubeProgrammer voit alors plusieurs
     segments a la meme adresse physique et affiche "File corrupted. Two or
     more segments define the same memory zone". Sans AT>, LMA = VMA (comme
     dans le script du CM7) et le binaire produit est inchange. */
  
  .shared (NOLOAD):
  {
    . = ALIGN(4);
    KEEP (*(.shared_var));  
    KEEP (*(.shared_config));
    . = ALIGN(4); 
  } >RAM_SHARED

  .shared_cached (NOLOAD):
  {
    . = ALIGN(4);
    KEEP (*(.rgbBuffers));
    KEEP (*(.imuData));    
    . = ALIGN(4);    
  } >RAM_SHARED_CACHED
  
  .ram_d4_shared (NOLOAD):
  {
    . = ALIGN(4);
    KEEP(*(.scanline_CM4));
    . = ALIGN(4);
  } >RAM_D4
  
  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
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
  } >RAM_D1A AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1A

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_D1A

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}


