/*
* PS1USB libsn_usb by OrionSoft [2024]
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
	bool	is_enabled;
	int		(* p_USB_Process)(uint8_t cmd, uint32_t param, uint8_t *data, uint32_t size, uint32_t *ret);
} KernelHookParams;

// Kernel hook params are defined at the end of available reserved kernel space
// The address can be decreased to get more room if needed
// The linker script in the kernel_hook folder would need to be adjusted too
static KernelHookParams	*kernel_hook_params = (KernelHookParams *)0x8000DF70;

/****************************************/
// PS1 USB Data Transfer Protocol

/*
** re-initialise PC filing system, close open files etc
**
** passed: void
**
** return: error code (0 if no error)
*/
int	PCinit(void)
{
	if (!kernel_hook_params->p_USB_Process('I', 0, NULL, 0, NULL))
		return (-1);
	return (0);
}

/*
** open a file on PC host
**
** passed:	PC file pathname, open mode, permission flags
**
** return:	file-handle or -1 if error
**
** note: perms should be zero (it is ignored)
**
** open mode:	0 => read only
** 				1 => write only
**					2 => read/write
*/
int	PCopen(char *name, int flags, int perms)
{
	int	ret;

	if (!kernel_hook_params->p_USB_Process('O', flags, name, strlen(name)+1, &ret))
		return (-1);
	return (ret);
}

/*
** create (and open) a file on PC host
**
** passed:	PC file pathname, open mode, permission flags
**
** return:	file-handle or -1 if error
**
** note: perms should be zero (it is ignored)
*/
int	PCcreat(char *name, int perms)
{
	int	ret;

	if (!kernel_hook_params->p_USB_Process('C', 0, name, strlen(name)+1, &ret))
		return (-1);
	return (ret);
}

/*
** seek file pointer to new position in file
**
** passed: file-handle, seek offset, seek mode
**
** return: absolute value of new file pointer position
**
** (mode 0 = rel to start, mode 1 = rel to current fp, mode 2 = rel to end)
*/
int	PClseek(int fd, int offset, int mode)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!kernel_hook_params->p_USB_Process('S', (mode << 24) | (fd & 0xFFFFFF), NULL, offset, &ret))
		return (-1);
	return (ret);
}

/*
** read bytes from file on PC
**
** passed: file-handle, buffer address, count
**
** return: count of number of bytes actually read
**
** note: unlike assembler function this provides for full 32 bit count
*/
int	PCread(int fd, char *buff, int len)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!kernel_hook_params->p_USB_Process('R', fd, buff, len, &ret))
		return (-1);
	return (ret);
}

/*
** write bytes to file on PC
**
** passed: file-handle, buffer address, count
**
** return: count of number of bytes actually written
**
** note: unlike assembler function this provides for full 32 bit count
*/
int	PCwrite(int fd, char *buff, int len)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!kernel_hook_params->p_USB_Process('W', fd, buff, len, &ret))
		return (-1);
	return (ret);
}

/*
** close an open file on PC
**
** passed: file-handle
**
** return: negative if error
**
*/
int	PCclose(int fd)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!kernel_hook_params->p_USB_Process('c', fd, NULL, 0, &ret))
		return (-1);
	return (ret);
}
