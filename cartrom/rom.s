# LibPSn00b Example Programs
# Part of the PSn00bSDk project
#
# TurboBoot Example by Lameguy64
#
# Note: This example is being obsoleted as GAS is not ideal for making
# ROM firmwares. Use ARMIPS instead, but it cannot build this example
# as it is not GAS syntax compatible.


# Uncomment either PAR or XPLORER depending on the cartridge
# you're going to use (makes disabling turbo boot via switch to work)

#.set PAR, 0
#.set XPLORER, 1


.set noreorder

.include "cop0.inc"				# Contains definitions for cop0 registers

.set SP_base,		0x801ffff0
.set BREAK_ADDR,	0xa0000040	# cop0 breakpoint vector address


.set RAM_buff,		2048
.set RAM_handle,	2052
.set RAM_tcb,		2056
.set RAM_evcb,		2060
.set RAM_stack,		2064
.set RAM_psexe,		2068


.set EXE_pc0,		0			# PS-EXE header offsets
.set EXE_gp0,		4
.set EXE_taddr,		8
.set EXE_tsize,		12
.set EXE_daddr,		16
.set EXE_dsize,		20
.set EXE_baddr,		24
.set EXE_bsize,		28
.set EXE_spaddr,	32
.set EXE_sp_size,	36
.set EXE_sp,		40
.set EXE_fp,		44
.set EXE_gp,		48
.set EXE_ret,		52
.set EXE_base,		56
.set EXE_datapos,	60


.section .text


# ROM header
#
# The Licensed by... strings are essential otherwise the BIOS will not
# execute the boot vectors. Always make sure the tty message fields (string
# after Licensed by) must be no more than 80 bytes long and must have a null
# terminating byte.
#
# Postboot vector isn't particularly practical as its only executed in between
# the PS boot logo and the point where game execution occurs.
#
header:
	# Postboot vector
	.word	0
	.ascii	"PS1USB by   OrionSoft [2024]"
	.ascii	"This product is NOT            "

	# Preboot vector
	.word	preboot
	.ascii	"Licensed by Sony Computer Entertainment Inc."

	.balign	0x80	# This keeps things in the header aligned
	
	
# Preboot vector
#
# All it does is it initializes a breakpoint vector at 0x40
# and sets a cop0 breakpoint at 0x80030000 to perform a midboot
# exploit as preboot doesn't have the kernel area initialized.
#
preboot:
	
	li		$v0, 1
	
.ifdef XPLORER
	lui		$a0, 0x1f06				# Read switch status for Xplorer
	lbu		$v0, 0($a0)
.endif

.ifdef PAR
	lui		$a0, 0x1f02				# Read switch status for PAR/GS devices
	lbu		$v0, 0x18($a0)
.endif

	nop
	andi	$v0, 0x1
	beqz	$v0, .no_rom			# If switch is off don't install hook
	nop								# and effectively disables the cartridge
	
	li		$v0, BREAK_ADDR			# Patch a jump at cop0 breakpoint vector
	
	li		$a0, 0x3c1a1f00			# lui $k0, $1f00
	sw		$a0, 0($v0)
	la		$a1, entry				# ori $k0, < address to code entrypoint >
	andi	$a1, 0xffff
	lui		$a0, 0x375a
	or		$a0, $a1
	sw		$a0, 4($v0)
	li		$a0, 0x03400008			# jr  $k0
	sw		$a0, 8($v0)
	sw		$0 , 12($v0)			# nop
	
	lui		$v0, 0xffff				# Set BPCM and BDAM masks
	ori		$v0, 0xffff
	mtc0	$v0, BDAM
	mtc0	$v0, BPCM
	
	
	li		$v0, 0x80030000			# Set break on PC and data-write address
	
	mtc0	$v0, BDA				# BPC break is for compatibility with no$psx
	mtc0	$v0, BPC				# as it does not emulate break on BDA
	
	lui		$v0, 0xeb80				# Enable break on data-write and and break
	mtc0	$v0, DCIC				# on PC to DCIC control register
	
.no_rom:

	jr		$ra						# Return to BIOS
	nop


# Actual ROM entrypoint
.global entry
entry:
	
	mtc0	$0 , DCIC				# Clear DCIC register
	
	la		$sp, SP_base			# Set stack base
	la		$gp, 0x8000c000			# Set GP address as RAM base addr in this case
	
	jal		SetDefaultExitFromException		# Set default exit handler just in case
	nop
	jal		ExitCriticalSection		# Exit out of critical section (brings back interrupts)
	nop

#	jal		_96_init				# Initialize the CD
#	nop

	la		$a1,psexe		# src
	lw		$a0,0x18($a1)		# Where we'll write to
	la		$a2,psexe_end
	addiu		$a0,$a0,-0x800		# header
	subu		$a2,$a1
	move		$s0,$a0
	addiu		$t2, $0, 0xA0
	jal		$t2
	addiu		$t1, $0, 0x2A		# memcpy

	jal		EnterCriticalSection	# Disable interrupt handling
	nop

	addiu		$a0,$s0,0x10		# PS-X EXE -> Header
	move		$a1,$0
	move		$a2,$0
	jal		DoExec
	addiu		$sp,-12
	addiu		$sp,12

.include "bios.inc"

	.balign 4
psexe:
	.incbin	"../ps1usb.ps-exe"
psexe_end:
	.org	0x00010000
	.incbin	"tim.tfs"

