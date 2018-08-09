/*
 * ===========================================================================
 *
 *       Filename:  disk.h
 *
 *    Description:  File backed disk-like device
 *
 *        Version:  1.0
 *        Created:  12-11-2015 13:00:03
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rodrigo Virote Kassick (), kassick@gmail.com
 *   Organization:  
 *
 * ===========================================================================
 */


#ifndef __DISK_H__
#define __DISK_H__

#include <stdlib.h>
#include <stdio.h>

typedef struct _disk {
    char * backing_file_name;
    int backing_fd;
    ssize_t size; // in sectors
} disk_t;

#define SECTOR_SIZE 4096

disk_t* disk_new(const char * backing_file_name, ssize_t size);
int disk_read_sector(disk_t* disk, off_t sect_number, unsigned char out_data[SECTOR_SIZE]);
int disk_write_sector(disk_t* disk, off_t sect_number, unsigned char in_data[SECTOR_SIZE]);
int disk_dispose(disk_t * disk);



#endif
