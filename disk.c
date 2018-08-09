/*
 * ===========================================================================
 *
 *       Filename:  disk.c
 *
 *    Description:  File-backed disk-like device
 *
 *        Version:  1.0
 *        Created:  12-11-2015 12:59:57
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rodrigo Virote Kassick (), kassick@gmail.com
 *   Organization:  
 *
 * ===========================================================================
 */

#include "disk.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

// For Windows, uncomment the following code
/*int pread(unsigned int fd, char *buf, size_t count, int offset)
{
	if (_lseek(fd, offset, SEEK_SET) != offset) {
		return -1;
	}
	return read(fd, buf, count);
}
int pwrite(unsigned int fd, char *buf, size_t count, int offset)
{
	if (_lseek(fd, offset, SEEK_SET) != offset) {
		return -1;
	}
	return write(fd, buf, count);
}
*/


/*---------------------------------------------------------------------------
 *  Function: disk_new
 *  Description: Creates a new "disk".
 *  Return value: NULL in case of error. A new disk_t object, otherwise
 *--------------------------------------------------------------------------*/
disk_t* disk_new(const char * backing_file_name, ssize_t size)
{
    disk_t * ret = NULL;

    FILE* f = fopen(backing_file_name, "r+");
    if (!f) // probably does not exist
        f = fopen(backing_file_name, "w+");
    if (!f) {
        fprintf(stderr, "Could not open file %s: %s\n", backing_file_name,
                strerror(errno));
        return NULL;
    }

    ret = malloc(sizeof(disk_t));
    ret->backing_fd = fileno(f);
    ret->backing_file_name = strdup(backing_file_name);
    ret->size = size;

    return ret;
}


/*---------------------------------------------------------------------------
 *  Function: disk_read_sector
 *  Description: Reads a sector from the disk and outputs it in out_data
 *  Return value: -1 in error, 0 in case of success
 *--------------------------------------------------------------------------*/
int disk_read_sector(disk_t * disk, off_t sect_number, unsigned char out_data[SECTOR_SIZE])
{
    if (!disk) return -1;
    if (sect_number >= disk->size) return -1;

    bzero(out_data, sizeof(unsigned char)*SECTOR_SIZE);

    // reads SECTOR_SIZE bytes into out_data
    int bytes_read = pread( disk->backing_fd,   //read from this file
                            out_data,           // into this buffer
                            SECTOR_SIZE,        // this amount of bytes
                            sect_number * SECTOR_SIZE); // from this offset on

    // Some error occurred
    if (bytes_read < 0)
        return -1;

    return 0;
}

/*---------------------------------------------------------------------------
 *  Function: disk_read_sector
 *  Description: Reads a sector from the disk and outputs it in out_data
 *  Return value: -1 in error, 0 in case of success
 *--------------------------------------------------------------------------*/
int disk_write_sector(disk_t* disk, off_t sect_number, unsigned char in_data[SECTOR_SIZE])
{
    if (!disk) return -1;
    if (sect_number >= disk->size) return -1;
    int bytes_written = pwrite( disk->backing_fd,
                                in_data,
                                SECTOR_SIZE,
                                sect_number * SECTOR_SIZE);
    if (bytes_written < 0)
        return -1;

    return 0;
}


/*---------------------------------------------------------------------------
 *  Function: disk_dispose
 *  Description: Diskposes of a disk
 *--------------------------------------------------------------------------*/
int disk_dispose(disk_t * disk)
{
    if (!disk) return -1;
    close(disk->backing_fd);
    free(disk->backing_file_name);
    free(disk);

    return 0;
}
