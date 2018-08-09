/*
 * ===========================================================================
 *
 *       Filename:  tosco_fs.h
 *
 *    Description:  Tosco-FS -- Implementação de Referência de T3 SO1
 *
 *        Version:  1.0
 *        Created:  20-11-2015 10:46:31
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rodrigo Virote Kassick (), kassick@gmail.com
 *   Organization:  
 *
 * ===========================================================================
 */

#ifndef __TOSCO_FS_H__
#define __TOSCO_FS_H__

#include "disk.h"
#include "bitmap.h"

// Meu sistema de arquivos trabalha com blocos de 4K e ponto, não é
// configurável
#define BLOCK_SIZE 4096
#define TFS_MAX_NAME_LEN 31
#define TFS_MAX_OPEN_FILES 10

// Um block é um número, sem sinal
typedef long long block_nr_t ;

// Tamanho de arquivo é qq coisa entre 0 e NUMERO GRANDE
typedef long long tfs_size_t ; 

typedef struct tfs_attr {
    tfs_size_t tamanho;
} tfs_attr_t;


// Quantas entradas na tabela de um inode ?
// Dos 4k, tira o tamanho dos atributos e o resto são entradas de tabela
#define N_ENTRIES_INODE ((BLOCK_SIZE - sizeof(tfs_attr_t)) / sizeof(block_nr_t))

// Número de entradas no bloco de continuação: Todos os 4k
#define N_ENTRIES_INODE_CONT (BLOCK_SIZE / sizeof(block_nr_t))


/*---------------------------------------------------------------------------
 *  Struct: tfs_inode
 *  Description: Um i-node do tosco-fs é só uma tabela. Não tem propriedades.
 *               Meu i-node tem atributos nele; Se a tabela for maior do que
 *               cabe aqui, o i-node continua num bloco extra (i-node_cont)
 *--------------------------------------------------------------------------*/
typedef struct tfs_inode_cont {
    block_nr_t where;
    block_nr_t mapping[N_ENTRIES_INODE_CONT];
} tfs_inode_cont_t;

typedef struct tfs_inode {
    block_nr_t where; // Isso não vai no disco

    // Isso vai pro disco
    tfs_attr_t attrs;
    block_nr_t mapping[N_ENTRIES_INODE];
} tfs_inode_t;




/*---------------------------------------------------------------------------
 *  Struct: tfs_dir_entry
 *  Description: Uma entrada de diretório. Um nome de 32 bytes e um ponteiro
 *               para o i-node deste arquivo
 *--------------------------------------------------------------------------*/
typedef struct tfs_dir_entry {
    char name[TFS_MAX_NAME_LEN+1];
    block_nr_t inode_pos;
} tfs_dir_entry_t;



/*---------------------------------------------------------------------------
 *  Struct: d_entry
 *  Description: Um diretório, contém várias entradas (dir_entry)
 *               Guarda tantas entradas quanto cabe em um bloco de 4K
 *               4096 / (32 + 8) = 102  
 *--------------------------------------------------------------------------*/
#define N_ENTRIES_DENTRY (BLOCK_SIZE / sizeof(tfs_dir_entry_t))
typedef struct tfs_d_entry {
    tfs_dir_entry_t entries[N_ENTRIES_DENTRY];
} tfs_d_entry_t;



/*---------------------------------------------------------------------------
 *  Struct: tosco_fs
 *  Description: O super-block (VFS) do tosco-fs tem um ponteiro para o disco
 *               utilizado e um ponteiro para o diretório raiz
 *--------------------------------------------------------------------------*/
typedef struct tosco_fs {
    disk_t * disk;
    tfs_d_entry_t * dir;
    bitmap_t * occupied_blocks;
    block_nr_t sb_bloco_raiz;
    block_nr_t sb_nblocks;
} tosco_fs_t;

int tfs_mkfs(disk_t * d);
tosco_fs_t * tfs_mount(disk_t * d);
int tfs_create(tosco_fs_t * tfs, const char * name);
int tfs_delete(tosco_fs_t * tfs, const char * fname);

struct tfs_lsdir_entry {
    char name[TFS_MAX_NAME_LEN+1];
    tfs_size_t size;
    tfs_size_t blocks_used;
};

int tfs_list_dir(tosco_fs_t *tfs, struct tfs_lsdir_entry * out, int max);

int tfs_open_file(tosco_fs_t * tfs, const char * fname);
int tfs_close_file(tosco_fs_t * tfs, int fd);
int tfs_write(tosco_fs_t * tfs, int fd, long long offset, long long len, const char * buffer);
int tfs_read(tosco_fs_t * tfs, int fd, long long offset, long long len, char * buffer);
void tfs_info(tosco_fs_t * tfs);
void tfs_file_info(tosco_fs_t * tfs, int fd);
#endif
