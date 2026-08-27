
/*
** Return offset to current 128-byte record.
**
** Note from_cur[fd] is updated with the current position of the file
*/

#include "stdio.h"

/* if greater than zero return 1 */
ctell(fd) int fd; {
	return (lseek(fd, 0, SEEK_CUR)); /* Return current file position */
  }

/*
** Return offset to next byte in current 128-byte record.
*/
ctellc(fd) int fd;{
  return (lseek(fd, 0, SEEK_CUR) & 127);
  }
