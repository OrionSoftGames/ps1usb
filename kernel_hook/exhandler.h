/**
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
**/
/**

PS1 Crash Handler

Requirements: Generally, be using the built-in ROM routines and exception
handling. This uses the A(0x4E) call, the task control block at 0x108, and the
default exception handler that fills in the task info, and it hooks the A(0x40)
"unresolved exception" call. If you want to catch program exits, your _exit
routine should cause an exception, with e.g. the "break" instruction. Newlib
libc does this in crt0.S.

To use: Call InstallExceptionHandler to install the hook. Optionally call
PSXPrintError with an error description right before an intentional crash occurs
to add the error description to the crash display.

- Spencer Alves (impiaaa)

**/

static inline void syscall_gpu_sync(void) {
    register volatile int n asm("t1") = 0x4E;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    ((void(*)(void))0xA0)();
}

static volatile unsigned int* GP0 = (volatile unsigned int*)0xBF801810;
static volatile unsigned int* GP1 = (volatile unsigned int*)0xBF801814;

static int charX = 0;
static int charY = 0;
#define	charWidth	8
#define	charHeight	8
#define	screenWidth SCREEN_WIDTH

static bool hasDarkenedScreen = false;

// OrionSoft: Added an embedded tiny 8x8 font instead of relying on a larger BIOS font (not sure that the address used worked with every bios)
extern uint8_t _binary_font8x8_bin_start[];

// Location is hardcoded at the end of unused kernel space to make sure the original function address is valid so the hook can be uninstalled
static uint32_t	*p_UnresolvedExceptionOld = (uint32_t *)0x8000DF78;

static void DarkenScreen() {
    *GP1 = 0x01000000; // clear command buffer
    *GP1 = 0x04000000; // disable DMA
    *GP1 = 0x08000001|((((*GP1)&0x100000) != 0)<<3); // 320x240, no interlace, 15bpp, preserve video mode
    syscall_gpu_sync();
    *GP0 = 0xE1000400; // 50/50 blend mode, texture don't care, drawing allowed
    syscall_gpu_sync();
    *GP0 = 0xE3000000; // drawing area top-left 0,0
    syscall_gpu_sync();
    *GP0 = 0xE4000000|(256<<10)|(screenWidth); // drawing area bottom-right
    syscall_gpu_sync();
    *GP0 = 0xE5000000; // drawing offset top-left
    syscall_gpu_sync();
    *GP0 = 0xE6000000; // disable masking
    syscall_gpu_sync();
    *GP0 = 0x63000000; // black, semitransparent rectangle
    *GP0 = 0x00000000; // top-left
    *GP0 = (256<<16)|(screenWidth); // full screen
}

static void PrintCharacter(const char c) {
    if (c == '\b') {
        charX -= charWidth;
        if (charX < 0) {
            charX = screenWidth-8;
            charY -= charHeight;
        }
    }
    else if (c == '\n') {
        charX = 0;
        charY += charHeight;
    }
    else if (c == '\r') {
        charX = 0;
    }
    else {
        if (c == '\t') {
            charX = (charX+charWidth)&0xFFE0;
        }
        else if (c == ' ') {
            charX += charWidth;
        }
        else if (c >= 0x21 && c <= 0x7E) {
            for (int row = 0; row < charHeight; row++) {
                unsigned int bits = _binary_font8x8_bin_start[charHeight*(c - 0x21) + row];
                for (int col = charWidth-1; col >= 0 && bits != 0; col--) {
                    if (bits&1) {
                        syscall_gpu_sync();
                        *GP0 = 0x69FFFFFF; // white pixel
                        *GP0 = ((charY+row)<<16)|(charX+col);
                    }
                    bits >>= 1;
                }
            }
            charX += charWidth;
        }
        else {
            return;
        }
        if (charX >= screenWidth) {
            charX = 0;
            charY += charHeight;
        }
    }
}

static void PrintHex(unsigned x) {
    for (int i = 0; i < 8; i++) {
        int c = x>>28;
        PrintCharacter(c > 9 ? c-10+'A' : c+'0');
        x <<= 4;
    }
}

// Public API: Call this before right calling abort(), exit() etc. to add a
// message to the screen before the crash info
void PSXPrintError(const char* s) {
    if (!hasDarkenedScreen) {
        hasDarkenedScreen = true;
        DarkenScreen();
    }
    if (s == (const char*)0) return;
    while (*s) {
        PrintCharacter(*s);
        s++;
    }
}

static const struct TCBH** taskControlBlockHeaderPointer = (const struct TCBH**)0x108;

static void (* oldHandler)();

// OrionSoft: Added cause string display
char *cause_str[] = {
	"Interrupt",
	"TLB Modify",
	"TLB miss on load",
	"TLB miss on store",
	"Address error on load",
	"Address error on store",
	"Bus error on instruction fetch",
	"Bus error on data access",
	"System call",
	"Breakpoint instruction",
	"Reserved (illegal) instruction",
	"Coprocessor unusable",
	"Integer overflow",
	"Reserved"
};

// OrionSoft: Added registers display
char reg_str[66+1] = "PCSRATV0V1A0A1A2A3T0T1T2T3T4T5T6T7S0S1S2S3S4S5S6S7T8T9K0K1GPSPFPRA";

static void UnresolvedException()
{
	int	i;

    // BIOS exception handler clobbers $gp; restore it
    // OrionSoft: The compiler couldn't find _gp so I used the stack instead (not sure of this)
    asm("addiu $sp, -32");
    asm("move $gp, $sp");

    // if the exception came from _exit, a0 has the passed exit code
    struct TCB* task = (*taskControlBlockHeaderPointer)->entry;

    // cop0r13/cause at the time of the exception
    PSXPrintError("\n CAUSE: ");
    i = (task->reg[R_CAUSE] >> 2) & 0xF;
	PSXPrintError(cause_str[(i < 13) ? i : 13]);

	// OrionSoft: Added a nice display of the registers
	for (i = 0; i < 33; i++)
	{
		char	reg[4];
		int		r = (i == 0) ? R_EPC : ((i == 1) ? R_SR : i - 2 + 1);

		if (i <= 16)
		{
			charX = charWidth;
			charY = charWidth*(i+4);
		}
		else
		{
			charX = charWidth*14;
			charY = charWidth*((i-16)+4);
		}
		reg[0] = reg_str[i+i];
		reg[1] = reg_str[i+i+1];;
		reg[2] = '=';
		reg[3] = '\0';
		PSXPrintError(reg);
		PrintHex(task->reg[r]);
	}

    // dump stack
    unsigned* sp = (unsigned *)task->reg[29];
    charX = charWidth*26;
    charY = charHeight*3;
	PSXPrintError("Stack dump:\n");
    while ((((unsigned)sp)&(~0xE0000000)) >= 0 && (((unsigned)sp)&(~0xE0000000)) < 0x200000 && charY < 256) {
		charX = charWidth*27;
        PrintHex(*sp);
        PrintCharacter('\n');
        sp++;
    }

    // ensure display
    syscall_gpu_sync();
    *GP1 = 0x05000000; // top-left corner
    *GP1 = 0x03000000; // enable display

    // continue to the existing handler
    // by default it calls the same handler, which since we've redirected that
    // here, could overflow the stack. a debugger could still hook the same
    // function though and we'd like to fall through, so check for the default implementation
    if (((unsigned *)oldHandler)[0] != 0x240A00A0 ||
        ((unsigned *)oldHandler)[1] != 0x01400008 ||
        ((unsigned *)oldHandler)[2] != 0x24090040) {
        oldHandler();
    }
    // if it was the default implementation, or it returned control, hang
    while(1) {
        // some emulators only refresh the display if the GPU is poked
        volatile int foo = *GP1;
    }
}

static void (** const a0table)() = (void (**)())0x200;

// Public API: Call this to start catching crashes
void InstallExceptionHandler() {
    oldHandler = a0table[0x40];
    a0table[0x40] = UnresolvedException;
    *p_UnresolvedExceptionOld = (uint32_t)UnresolvedException;
}
