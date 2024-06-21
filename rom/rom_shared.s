#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
#

#
# ROMProd 0.4, 12/12/20, sickle
# https://github.com/JonathanDotCel
#
# - make rom
# - make exe
# - glue exe to rom
# - flash to a thing
# - instaboot exe
#   

#
# There may be some odd conventions and far too many nops, but it's
# 2020 and readability sits higher on my list than performance here
#


            #.section .start, "ax", @progbits
            .section .text

            .global __start

            .set noreorder

__start:

            #
            # EP1 -> Diamond Screen -> Black screen -> EP2 -> game start
            #

            #
            # Note: Unlike standalone assemblers, these addresses are 
            #       relative to the org specified in your .mk file.
            #       E.g. here EP2 is 0x1F000000 + 0x00000080 = 0x1F000080
            #

            # EP2: (Disabled)            
            .org        0x00000000                                          #header, points to EP2
            .long       0
            .string     "PS1USB by   OrionSoft [2024]"

            .org        0x00000080-32
            .string     "This product is NOT            "

            # Between EPs. We can actually put some code here
            # such as that used by AHOY! to stop the CD            

            # EP1: (Enabled)            
            .org        0x00000080      #0x80 points to EP1
            .long       entryPoint1
            .string     "Licensed by Sony Computer Entertainment Inc."

	.align	4

            # This would be where you could start inserting your own code.
            

        .set noreorder

            # Entry point 1 is after the PS logo.
            # it's been disabled by using an invalid header in 
            # rom_compressed and rom_uncompressed
entryPoint1:

            # The instant-on boot point.
            # we can get in before the kernel and set a hook point
            # for when the shell is loaded to 0x8003000

entryPoint2:

            # remove any cop0 hooks
            mtc0    $0, $7      #Remove the handler  r7/a3
            nop
        
            # write 0xFFEE2244 to 0x80100060 before rebooting 
            # if you want to do a normal boot

            lui     $t0, 0x8010
            li      $t1, 0xFFEE2244
            lw      $t2, 0x60($t0)
            nop
            bne     $t1, $t2, noReturn
            nop

            # stop it returning next time
            sb      $0, 0x60($t0)
            nop

            jr $ra
            nop

noReturn:

        


noSwitch:

    # Inits the mid-boot execution hook using cop0
    # the BIOS will unpack the shell to 0x80030000
    # We set an execution hook there and take over
    # execution before it displays anything, but
    # after it's installed the kernel, setup the 
    # jump tables, installed (most) devices, etc
    #

midBootHook:

        # source, dest
        la      $t0, reVector
        li      $t1, 0x80000040
        nop

        # no point writing a copy loop for 4 dwords
        # this copies the function into addr 0x80000040
        # where the CPU will jump as the hook activates
        lw      $t2, 0x0($t0)
        lw      $t3, 0x4($t0)
        lw      $t4, 0x8($t0)
        lw      $t5, 0xC($t0)
        nop
        sw      $t2, 0x0($t1)
        sw      $t3, 0x4($t1)
        sw      $t4, 0x8($t1)
        sw      $t5, 0xC($t1)
        nop


    # Set a break point after the kernel's (mostly)
    # finished setting things up, but before it
    # wastes time unpacking the GUI.
    # This patch dedicated to Nicolas Noble's unrelenting perfectionism.

    # Exec Breakpoint   0100 0000  ( bit 24 ) BPC, BPCM
    # Data Accss        0200 0000  ( bit 25 ) BDA, BDAM
    # Data Read         0400 0000  ( bit 26 ) ( requires bit 25 HI )
    # Data Write        0800 0000  ( bit 27 ) ( requires bit 25 HI )


    #BP on Data access

        li      $t0, 0x80030000
        mtc0    $t0, $5             # reg5 = BPC/Breakpoint on Access
        nop

        li      $t0, 0xFFFFFFFF
        mtc0    $t0, $9             # reg9 = BDAM/Breakpoint on Access Address Mask
        nop

    # BP on Exec (old way)

        mtc0    $t0, $3             # reg3 = BPC/Breakpoint on Exec
        nop

        li      $t0, 0xFFFFFFFF
        mtc0    $t0, $11                # reg11 = BPCM/Breakpoint on Exec Mask
        nop

        # The exec hook is there to support no$psx
        #li     t0, 0xE1800000      # Exec BP
        #li     t0, 0xEA800000      # Write BP
        li      $t0, 0xEB800000     # Access + Exec BP
        mtc0    $t0, $7             # a3 = reg7 = DCIC/Breakpoint Control
        nop
        
        jr      $ra
        nop

reVector:
        la      $t0, start
        jr      $t0
        nop
        nop

        # Now we return to the BIOS and let it set up the kernel...
    
    
        

        #
        # Ah you're back!
        #
        # The BIOS set everything (most things) up and started
        # unpacking the shell. This triggered the hook and
        # brought us here.
        #

start:

        # remove the hook!
        mtc0    $0, $7
        nop
        mtc0    $0, $9
        nop
        
        # Run either the compressed or uncompressed loader
        # from the rom_compressed.s or rom_uncompressed.s
        la      $a0, exe_01  
        jal     LoadMe
        nop
        

            

            


