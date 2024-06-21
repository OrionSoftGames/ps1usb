#ifndef _LIBSN_USB_H_
#define _LIBSN_USB_H_

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

/*
** re-initialise PC filing system, close open files etc
**
** passed: void
**
** return: error code (0 if no error)
*/
int	PCinit (void);

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
int	PCopen (char *name, int flags, int perms);

/*
** create (and open) a file on PC host
**
** passed:	PC file pathname, open mode, permission flags
**
** return:	file-handle or -1 if error
**
** note: perms should be zero (it is ignored)
*/
int	PCcreat (char *name, int perms);

/*
** seek file pointer to new position in file
**
** passed: file-handle, seek offset, seek mode
**
** return: absolute value of new file pointer position
**
** (mode 0 = rel to start, mode 1 = rel to current fp, mode 2 = rel to end)
*/
int	PClseek (int fd, int offset, int mode);

/*
** read bytes from file on PC
**
** passed: file-handle, buffer address, count
**
** return: count of number of bytes actually read
**
** note: unlike assembler function this provides for full 32 bit count
*/
int	PCread (int fd, char *buff, int len);

/*
** write bytes to file on PC
**
** passed: file-handle, buffer address, count
**
** return: count of number of bytes actually written
**
** note: unlike assembler function this provides for full 32 bit count
*/
int	PCwrite (int fd, char *buff, int len);

/*
** close an open file on PC
**
** passed: file-handle
**
** return: negative if error
**
*/
int	PCclose (int fd);


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif /* _LIBSN_H_ */
