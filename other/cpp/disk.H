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


#pragma once

#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

#define SECTOR_SIZE 4096

class Disk {
    public:
        string backing_file_name;
        std::fstream backing_file;
        ssize_t size; // in sectors


        /*---------------------------------------------------------------------------
         *  Function: Disk::ctor
         *  Description: Construct new disk-like object, initialize it's
         *  internal fields and tryies to open the file.
         *  Throws: -1 on error
         *--------------------------------------------------------------------------*/
        Disk(const char* _backing_file_name, ssize_t _size):
            backing_file_name(_backing_file_name),
            size(_size)
        {
            backing_file.open(_backing_file_name,
                              ios::in|ios::out|ios::binary);

            if (!backing_file.is_open()) 
                // probably does not exist, create it
                backing_file.open(_backing_file_name,
                                  ios::trunc|ios::in|ios::out|ios::binary);

            if (!backing_file.is_open()) {
                throw -1;
            }
        }


        /*---------------------------------------------------------------------------
         *  Function: Disk::dtor
         *  Description: Disk destructor -- closes the backing file. Not
         *  needed, just here for explicity
         *--------------------------------------------------------------------------*/
        ~Disk() {
            backing_file.close();
        }


        /*---------------------------------------------------------------------------
         *  Function: Disk::read_sector
         *  Description: Reads a sector from the backing file
         *  Throws: May throw failure (ios/io_base) in case error occurs on
         *          backing file
         *  Returns: 0 on success, -1 on failure
         *--------------------------------------------------------------------------*/
        int read_sector(off_t sect_number, char *out_data)
        {
            if (sect_number >= this->size) {
                cerr << "Out of bounds" << endl;
                return -1;
                }

            bzero(out_data, sizeof(char)*SECTOR_SIZE);

            // reads SECTOR_SIZE bytes into out_data
            try {
                backing_file.seekg(sect_number*SECTOR_SIZE);
                backing_file.read(out_data, SECTOR_SIZE);
            } catch (const ios_base::failure &f) {
                cerr << "Could not read: " << f.what() << endl;
                return -1;
            }

            return 0;

        }
        int write_sector(off_t sect_number, char* in_data) {
            if (sect_number >= this->size) return -1;

            try {
                backing_file.seekp(sect_number*SECTOR_SIZE);
                backing_file.write(in_data, SECTOR_SIZE);
            } catch (const ios_base::failure &f) {
                cerr << "Could not write: " << f.what() << endl;
                return -1;
            }

            return 0;
        }
};

