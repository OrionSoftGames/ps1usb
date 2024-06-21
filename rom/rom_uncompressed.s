# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

#
# The binary header is in rom_shared.s
# (for compressed and uncompressed .exe versions)
#

l_shared:
            .include    "rom_shared.s"
            .align 4



#
#   Copy an uncompressed .exe from ROM into RAM
#   Reads in the location of the .EXE relative to the rom , then adds 0x800
#   for the first chunk of executable code. This is written to 0x80010000 in
#   most cases.
#   I.e.  0x1f000000 + 1000 (rom size) + 800 (exe header) writes out to 0x80010000
#
#   Note: does not clear .bss, registers, etc.
#

# a0 = where's the data
#
LoadMe:

            lw      $t1, 0x10($a0)              # Not super necessary I guess.
            lw      $t2, 0x18($a0)              # Where we'll write to
            lw      $t3, 0x1c($a0)              # Read the filesize
            addiu   $t4, $a0,0x800          # PSX EXE actual code offset (reading from)

keepReading:

            lb      $t5, 0x0($t4)               # read byte into t5
            nop
            addiu   $t4, $t4,0x1                # increment read offset

            sb      $t5, 0x0($t2)               # write t5 back out to the memory
            nop
            addiu   $t2, $t2,0x1                # increment the write offset

            sub     $t3, 0x1                    # subtract 1 from filesize (negative counter)

            bne     $t3, $0, keepReading    # Keep reading until we've hit 0
            nop

            lw      $t1, 0x10($a0)              # where we're about to jump to
            lw      $sp, 0x30($a0)              # The stack pointer. (was 0x2c, not iportant, but wrong)
            lw      $gp, 0x3c($a0)              # 0x00000000 in every exe I've seen.

            # Could return to XFlash or whatever now if you want to dump mem
            # but let's just boot the .exe

            jr      $t1                         # Jumpity jump jump
            nop


# marker for debugging in IDA/Hex view
binmark:
            .long	0xCEDBCEDB
            .long	0x1337C0DE
            nop
            .align 4

#
# The end of the assembly; myfile.exe is glued on here
#
exe_01:
            
            .incbin "../ps1usb.ps-exe"
            .align 4

            # If you're appending a file to the end manually (cat/copy/etc)
            # remember to check that the compiler isn't adding extra bytes!
            # assemblers generally won't, the GNU toolchain does.

            .org        0x00010000
            .incbin "tim.tfs"

