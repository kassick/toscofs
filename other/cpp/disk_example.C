/*
 * ===========================================================================
 *
 *       Filename:  disk_example.c
 *
 *    Description:  Example of using disk.c
 *
 *        Version:  1.0
 *        Created:  12-11-2015 13:34:27
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rodrigo Virote Kassick (), kassick@gmail.com
 *   Organization:  
 *
 * ===========================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include "disk.H"

#define TEST1 "THIS IS A TEST"
#define TEST2 "THIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLA"
#define TEST3 "WHAT || EVER"
#define TEST4 "THIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLTHIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLTHIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLTHIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLTHIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLTHIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLTHIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BIGGER BLA BLA BLAAAAAAATHIS IS A CONTENT OF A SECTOR THAT I WANT TO BE BA"

using namespace std;

int main(int argc, char *argv[])
{
    char *sector = new char[SECTOR_SIZE];
    // My disk has 4 MBytes, thus 8192 sectors
    ssize_t disk_size = 4*1024*1024 / SECTOR_SIZE;

    assert(sizeof(char) == 1); // Make sure we are using 1-byte characters

    Disk d("disk.data", disk_size);

    return 1;

    // Sector 10 -> "THIS IS A TEST\0\0\0\0\0...."
    bzero(sector, SECTOR_SIZE);
    memcpy(sector, TEST1, strlen(TEST1));
    cerr << "sector is " << sector << endl;
    if (d.write_sector(10, sector)) {
        perror("Error writing sector 10");
        exit(1);
    }

    // Sector 0 -> "THIS IS A CONTENT OF A SEC..."
    bzero(sector, SECTOR_SIZE);
    memcpy(sector, TEST2, strlen(TEST2));
    if(d.write_sector(0, sector)) {
        perror("Error writing sector 0");
        exit(1);
    }
    
    // Sector 1 -> "WHAT || EVER"
    bzero(sector, SECTOR_SIZE);
    memcpy(sector, TEST3, strlen(TEST3));
    if (d.write_sector(1, sector)) {
        perror("Error writing sector 1");
        exit(1);
    }

    // CHeck sector 0: TEST2
    
    if (d.read_sector(0, sector)) {
        perror("Error reading back sector 0");
        exit(1);
    }

    if (strcmp((char*)sector, TEST2) != 0) {
        /// Ooops
        printf("OOOPS: Sector 0, expected %s, got %s\n",
                TEST2, sector);
        exit(1);
    }



    // CHeck sector 1: TEST3
    
    if (d.read_sector(1, sector)) {
        perror("Error reading back sector 0");
        exit(1);
    }

    if (strcmp((char*)sector, TEST3) != 0) {
        /// Ooops
        printf("OOOPS: Sector 1, expected %s, got %s\n",
                TEST3, sector);
        exit(1);
    }

    // Write sector 0 again
    if (strlen(TEST4) > 512) 
    {
        printf("OOPS: Big string must be, at most, 512 bytes long");
        exit(1);
    }

    bzero(sector, SECTOR_SIZE);
    memcpy(sector, TEST4, strlen(TEST4));
    if (d.write_sector(0, sector)) {
        perror("COuld not re-write sector 0\n");
        exit(1);
    }

    // CHeck sector 1: TEST3
    
    if (d.read_sector(1, sector)) {
        perror("Error reading back sector 0");
        exit(1);
    }

    if (strcmp((char*)sector, TEST3) != 0) {
        /// Ooops
        printf("OOOPS: Sector 1, expected %s, got %s\n",
                TEST3, sector);
        exit(1);
    }
    
    return 0;
}
