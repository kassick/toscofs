/*
 * ===========================================================================
 *
 *       Filename:  bitmap.c
 *
 *    Description:  Implementação de um bitmap
 *
 *        Version:  1.0
 *        Created:  20-11-2015 11:24:26
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rodrigo Virote Kassick (), kassick@gmail.com
 *   Organization:  
 *
 * ===========================================================================
 */

#include <math.h>
#include <stdlib.h>
#include "bitmap.h"
#include "string.h"
#include <stdio.h>



/*---------------------------------------------------------------------------
 *  Function: bitmap_new
 *  Description: Cria um novo bitmap sempre ZERADO
 *--------------------------------------------------------------------------*/
bitmap_t * bitmap_new(long long size)
{
    bitmap_t * r = malloc(sizeof(bitmap_t));
    long long buf_size;
    if (!r) return NULL;

    // quantos elementos na array?
    // 1 elemento -> 1 byte
    // 8 elementos -> 1 byte
    // 12 elementos -> 2 bytes
    // 16 elementos -> 2 bytes
    // 200 elementos -> 25 bytes
    buf_size = size / 8;
    if (size % 8)
        buf_size++;

    r->bm_array = malloc(buf_size);
    if (!r->bm_array) {
        free(r);
        return NULL;
    }

    // Inicializa tudo com "unset" (falso)
    // equivalente a for(i=0; i<buf_size; i++) bm_array[i] = 0;
    bzero(r->bm_array, buf_size);

    r->size = size;
    r->mem_size = buf_size;

    return r;
}


/*---------------------------------------------------------------------------
 *  Function: bitmap_get
 *  Description: Retorna se o bit na posição pos está 0 ou 1
 *--------------------------------------------------------------------------*/
int bitmap_get(bitmap_t * b, long long pos)
{
    if (!b || pos > b->size) {
        // Erro
        return -1;
    }

    long long vpos = pos / 8;
    int bit_pos = pos % 8;

    // Pega o byte que contém o bit
    unsigned char ret = b->bm_array[vpos];

    // Shiftar 1 para a esquerda:
    // 00000001  << 2  =>   00000100
    // Se o valor de ret (posição que contém o bit que queremos) contém
    // 01011100 e a posição do bit é 2 (3a posição da esq pra direita), 
    // vamos fazer
    // 01011100 &
    // 00000100 =
    // 00000100  (Diferente de zero)
    //
    // Se o valor de ret contém 01011000, 
    // 01011000 &
    // 00000100 =
    // 00000000 (Igual a zero)
    ret = ret & (1 << bit_pos);

    return ret;
}

int bitmap_set(bitmap_t * b, long long pos, int val)
{
    if (!b || pos > b->size) {
        // Erro
        return -1;
    }

    long long vpos = pos / 8;
    int bit_pos = pos % 8;

    // Pega o byte que contém o bit
    unsigned char org_val = b->bm_array[vpos];
    unsigned char mask = 1 << bit_pos;
    if (val) {
        // Estou setando o bit para 1
        // isso é um OR
        org_val = org_val | mask;
    } else {
        // Estou setando bit para 1
        // Isso é um AND com not
        // 01010101 , setar o bit posição 2 (3o da esq para a direita)
        // Pega a máscara 00000100 , inverte (11111011) e faz AND
        // 01010101 &
        // 11111011 =
        // 01010001
        
        org_val = org_val & (~mask);
    }

    b->bm_array[vpos] = org_val;

    return 0;
}

// Encontra a posição do primeiro bit em 1
long long bitmap_find_first_set(bitmap_t * b) {
    long long i;

    for (i = 0; i < b->size; i+= 8) {
        long long vpos = i/8;
        if (b->bm_array[vpos]) {
            // Aqui tem um bit em 1
            for (int bitpos = 0; bitpos < 8; bitpos++) {
                unsigned char mask = 1 << bitpos;
                if (b->bm_array[vpos] & mask) {
                    return i + bitpos;
                }
            }
        }
    }
    
    // Não tem
    return -1;
}

// Encontra a posição ro primeiro bit em 0
long long bitmap_find_first_unset(bitmap_t * b) {
    long long i;

    for (i = 0; i < b->size ; i+=8) {
        long long vpos = i/8;
        if (b->bm_array[vpos] != 255) {
            // Aqui tem um bit em 0
            // 00000001
            // 11111101
            for (int bitpos = 0; bitpos < 8; bitpos++) {
                unsigned char mask = ~(1 << bitpos);
                if ((b->bm_array[vpos] | mask) != 255) {
                    if ((i + bitpos) >= b->size)
                        return -1; // Encontrou 0s na área de sobra
                    else
                        return i + bitpos;
                }
            }
        }
    }
    
    // Não tem
    return -1;
}




// Compile com -DBM_UNIT_TEST para executar esses testes
#ifdef BM_UNIT_TEST

int main(int argc, char *argv[])
{
    bitmap_t * b = bitmap_new(201);
    if (!b) {
        printf("OOOPs, erro de mem\n");
        return 1;
    }

    if (bitmap_find_first_set(b) >= 0) {
        printf("OOOps, já achou alguem setado !?");
        return 1;
    }

    if (bitmap_find_first_unset(b) != 0) {
        printf("OOOPps, deveria ter encontrado 0 não-setado\n");
        return 1;
    }

    bitmap_set(b, 10, 1);
    bitmap_set(b, 110, 1);
    bitmap_set(b, 45, 1);

    if (bitmap_get(b, 10)) {
        printf("pos 10 ok\n");
    } else {
        printf("ERRO POS 10\n");
    }
    
    if (bitmap_get(b, 110)) {
        printf("pos 110 ok\n");
    } else {
        printf("ERRO POS 110\n");
    }
    
    if (bitmap_get(b, 45)) {
        printf("pos 45 ok\n");
    } else {
        printf("ERRO POS 45\n");
    }

    if (bitmap_get(b, 46)) {
        printf("pos 46 deveria estar unset!!!\n");
    } else {
        printf("pos 46 ok\n");
    }
    
    if (bitmap_get(b, 0)) {
        printf("pos 0 deveria estar unset!!!\n");
    } else {
        printf("pos 0 ok\n");
    }
    
    if (bitmap_get(b, 9)) {
        printf("pos 9 deveria estar unset!!!\n");
    } else {
        printf("pos 9 ok\n");
    }

    printf("Primeira posição com 0: %lld\n", bitmap_find_first_unset(b));
    printf("Primeira posição com 1: %lld\n", bitmap_find_first_set(b));

    printf("Seta pos 0 para 1\n");
    bitmap_set(b, 0, 1);
    printf("Primeira posição com 0: %lld\n", bitmap_find_first_unset(b));
    printf("Primeira posição com 1: %lld\n", bitmap_find_first_set(b));
    
    
    printf("Seta pos 0 para 0\n");
    bitmap_set(b, 0, 0);
    printf("Primeira posição com 0: %lld\n", bitmap_find_first_unset(b));
    printf("Primeira posição com 1: %lld\n", bitmap_find_first_set(b));

    printf("Seta pos 10 para 0\n");
    bitmap_set(b, 10, 0);
    printf("Primeira posição com 0: %lld\n", bitmap_find_first_unset(b));
    printf("Primeira posição com 1: %lld\n", bitmap_find_first_set(b));


    printf("Seta tudo entre 0 e 199 para 1\n");
    for (int i = 0; i < 200; i++) {
        bitmap_set(b, i, 1);
    }
    printf("Primeira posição com 0: %lld\n", bitmap_find_first_unset(b));
    printf("Primeira posição com 1: %lld\n", bitmap_find_first_set(b));
    
    
    printf("Seta 200 para 1\n");
    bitmap_set(b, 200, 1);
    printf("Primeira posição com 0: %lld\n", bitmap_find_first_unset(b));
    printf("Primeira posição com 1: %lld\n", bitmap_find_first_set(b));
    
    printf("Seta 46 para 0\n");
    bitmap_set(b, 46, 0);
    printf("Primeira posição com 0: %lld\n", bitmap_find_first_unset(b));
    printf("Primeira posição com 1: %lld\n", bitmap_find_first_set(b));


    return 0;
}

#endif
