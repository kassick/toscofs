/*
 * ===========================================================================
 *
 *       Filename:  bitmap.h
 *
 *    Description:  Implementação de um bitmap
 *
 *        Version:  1.0
 *        Created:  20-11-2015 11:28:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rodrigo Virote Kassick (), kassick@gmail.com
 *   Organization:  
 *
 * ===========================================================================
 */

#ifndef __BITMAP_H__
#define __BITMAP_H__

typedef struct bitmap {
    long long size;
    long long mem_size;
    unsigned char * bm_array;
} bitmap_t;

bitmap_t * bitmap_new(long long size);
int bitmap_get(bitmap_t * b, long long pos);
int bitmap_set(bitmap_t * b, long long pos, int val);

long long bitmap_find_first_set(bitmap_t * b);
long long bitmap_find_first_unset(bitmap_t * b);

// QUantos pedaços de chunk_size eu preciso para guardar esse bitmap?
static long bitmap_nchunks(bitmap_t * bm, long long chunk_size) {
    long bm_nblocks = bm->mem_size / chunk_size;
    if (bm->mem_size % chunk_size)
        bm_nblocks++;
    return bm_nblocks;
}
#endif

