#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <libapi.h>

#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	240

#include "../usb.h"
#include "ttyhook.h"
#include "exhandler.h"

void	(* ReturnFromExceptionOld)(void);
bool	is_tty_enabled;

typedef struct
{
	bool	is_enabled;
} KernelHookParams;

// Kernel hook params are defined at the end of some available reserved kernel space
// See linker script in the current folder
static KernelHookParams	kernel_hook_params __attribute__ ((section (".kernel_hook_params")));

// Location is hardcoded at the end of unused kernel space to make sure the original function address is valid so the hook can be uninstalled
static uint32_t	*p_ReturnFromExceptionOld = (uint32_t *)0x8000DF7C;

static inline void	**GetB0Table(void);
void	ReturnFromExceptionHook(void);
void	InstallUSBHook(void);

// Important: main() has to be the first defined function in order to be placed at the entry point 0x8000C160
void	main(void)
{
	InstallExceptionHandler();
	InstallUSBHook();
}

static inline void	**GetB0Table(void)
{
	register volatile int n asm("t1") = 0x57;
	__asm__ volatile("" : "=r"(n) : "r"(n));
	return ((void **(*)(void))0xB0)();
}

void	ReturnFromExceptionHook(void)
{
	int	ret;
	
	if (kernel_hook_params.is_enabled)
	{
		if (!is_tty_enabled)
		{
			SetupTTYhook();
			is_tty_enabled = true;
		}

		if (!(USB_STATUS & USB_STATUS_MASK_USB_READY))
			ret = USB_Process();
	}

	ReturnFromExceptionOld();
}

void	InstallUSBHook(void)
{
	void	**b0table = GetB0Table();

	ReturnFromExceptionOld = b0table[0x17];
	b0table[0x17] = ReturnFromExceptionHook;
	*p_ReturnFromExceptionOld = (uint32_t)ReturnFromExceptionOld;
}
