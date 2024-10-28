/*

MIT License

Copyright (c) 2019 PCSX-Redux authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

OUTPUT_FORMAT("binary")

EXTERN(_start)
ENTRY(_start)

TLOAD_ADDR = DEFINED(TLOAD_ADDR) ? TLOAD_ADDR : 0x80010000;

/*
Kernel hook part uses unused/reserved kernel space (to be safe, from 0x8000C160 to 0x8000DF7F)
https://problemkaputt.de/psx-spx.htm#biosmemorymap
https://github.com/Lameguy64/PSn00bSDK/blob/master/indev/psn00bdbg-mk2/doc/overview.txt
*/
MEMORY {
    ram (rwx)   : ORIGIN = 0x8000C160, LENGTH = 0x1E10
    khp (rw)    : ORIGIN = 0x8000DF70, LENGTH = 0x10
    dcache      : ORIGIN = 0x1f800000, LENGTH = 0x400
}

__ram_top       = ORIGIN(ram) + LENGTH(ram);
__sp            = __ram_top - 0x100;

__dcache        = ORIGIN(dcache);
__dcache_top    = ORIGIN(dcache) + LENGTH(dcache);

__bss_len       = (__bss_end - __bss_start);
__ftext_len     = (__ftext_end - __ftext_start);
__fdata_len     = (__fdata_end - __fdata_start);

__stack_start   = ORIGIN(ram) + LENGTH(ram);

SECTIONS {
    __ftext_start = ABSOLUTE(.);
    .text TLOAD_ADDR : {
        *(.start)
        *(.init)
        KEEP (*(SORT_NONE(.fini)))
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)

        . = ALIGN(16);
        KEEP(*(.init))
        . = ALIGN(16);
        KEEP(*(.fini))
    } > ram

    . = ALIGN(16);
    __text_end = .;
    __ftext_end = ABSOLUTE(.);

    __fdata_start = ABSOLUTE(.);

    .rodata : {
        *(.rodata .rodata.* .rdata .rdata.* .gnu.linkonce.r.*)
        . = ALIGN(16);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;

        . = ALIGN(16);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))

        . = ALIGN(16);
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        __init_array_end = .;

        . = ALIGN(16);
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))

        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        __fini_array_end = .;
        __build_id = .;
        *(.note.gnu.build-id)
        __build_id_end = .;
    } > ram

    .rodata1 : {
        *(.rodata1)
    } > ram

    __data_start = .;
    .data : {
        *(.a0table)
        *(.data .data.* .gnu.linkonce.d.*)
        *(.data1)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
        *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
        *(.got.plt)
        *(.got)
        /* pad file to be a multiple of 2048 bytes.  Needed for loading from CD-ROM. */
        . = ALIGN(2048);
    } > ram

    __data_end = .;
    __fdata_end = .;
    __bss_start = .;
    .sbss (NOLOAD) : {
        *(.dynsbss)
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
    } > ram
    
    .kernel_hook_params : {
        *(.kernel_hook_params)
    } > khp

    . = ALIGN(4);
    __bss_end = .;

    __heap_start = __heap_base;
	
	. = ADDR(.text) - 0x800;
    __end = .;

    /DISCARD/ : { *(.MIPS.abiflags) }

    /* Everything is statically linked, so discard PLTs. */
    /DISCARD/ : { *(.rel.iplt) *(.rela.iplt) *(.rel.plt) *(.rela.plt) *(.plt) *(.iplt) }

    /* Discard things that the standard link script drops, too. */
    /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
