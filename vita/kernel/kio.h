#ifndef _KIO_H_
#define _KIO_H_

/**
 * Open or create a file for reading or writing
 *
 * @par Example1: Open a file for reading
 * @code
 * SceUID fd;
 * kIoOpen("device:/path/to/file", SCE_O_RDONLY, &fd)
 * if(!(fd) {
 *  // error
 * }
 * @endcode
 *
 * @param file - Pointer to a string holding the name of the file to open
 * @param flags - Libc styled flags that are or'ed together
 * @param res - Pointer to where to save the returned file descriptor.
 *
 */
int kIoOpen(const char *file, int flags, SceUID* res);

/**
 * Delete a descriptor
 *
 * @code
 * kIoClose(fd);
 * @endcode
 *
 * @param fd - File descriptor to close
 *
 */
int kIoClose(SceUID fd);

/**
 * Read input
 *
 * @par Example:
 * @code
 * kIoRead(fd, data, 100);
 * @endcode
 *
 * @param fd - Opened file descriptor to read from
 * @param data - Pointer to the buffer where the read data will be placed
 * @param size - Size of the read in bytes
 *
 */
int kIoRead(SceUID fd, void *data, SceSize size);

/**
 * Write output
 *
 * @par Example:
 * @code
 * kIoWrite(fd, data, 100);
 * @endcode
 *
 * @param fd - Opened file descriptor to write to
 * @param data - Pointer to the data to write
 * @param size - Size of data to write
 *
 */
int kIoWrite(SceUID fd, const void *data, SceSize size);


/**
 * Reposition read/write file descriptor offset
 *
 * @par Example:
 * @code
 * kIoLseek(fd, -10, SEEK_END);
 * @endcode
 *
 * @param fd - Opened file descriptor with which to seek
 * @param offset - Relative offset from the start position given by whence
 * @param whence - Set to SEEK_SET to seek from the start of the file, SEEK_CUR
 * seek from the current position and SEEK_END to seek from the end.
 *
 */
int kIoLseek(SceUID fd, int offset, int whence);

/**
 * Remove directory entry
 *
 * @param file - Path to the file to remove
 *
 */
int kIoRemove(const char *file);

/**
 * Make a directory file
 *
 * @param dir
 * @param mode - Access mode.
 */
int kIoMkdir(const char *dir);

/**
 * Remove a directory file
 *
 * @param path - Removes a directory file pointed by the string path
 */
int kIoRmdir(const char *path);

/**
 * Return the position in the file
 *
 * @param fd - Opened file descriptor with which to seek
 * @param pos - The position in the file after the seek
 *
 */
int kIoTell(SceUID fd, SceOff* pos);

void kio_start();
void kio_stop();

#endif
