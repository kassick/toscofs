/*
 * ===========================================================================
 *
 *       Filename:  tosco_fs.c
 *
 *    Description:  Tosco-FS -- Implementação de Referência de T3 SO1
 *
 *        Version:  1.0
 *        Created:  20-11-2015 10:33:22
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
#include "disk.h"
#include "tosco_fs.h"

// Isto simula a parte de abrir e fechar arquivos da VFS
tfs_inode_t * tfs_opened_files[TFS_MAX_OPEN_FILES];



#define MIN(a,b) ( (a < b) ? a : b )
#define MAX(a,b) ( (a > b) ? a : b )

int tfs_write_dentry(tosco_fs_t * tfs, block_nr_t bn, tfs_d_entry_t * dentry);
int tfs_read_dentry(tosco_fs_t * tfs, block_nr_t bn, tfs_d_entry_t * dentry);
int tfs_dentry_add_entry(tfs_d_entry_t * dentry, const char * name, block_nr_t bn);
int tfs_dentry_del_entry(tfs_d_entry_t * dentry, int pos);
int tfs_read_inode(tosco_fs_t * tfs, block_nr_t bn, tfs_inode_t * inode);
int tfs_write_inode(tosco_fs_t * tfs, tfs_inode_t * inode);
int tfs_read_inode_cont(tosco_fs_t * tfs, block_nr_t bn, tfs_inode_cont_t * inode_cont);
int tfs_write_inode_cont(tosco_fs_t * tfs, tfs_inode_cont_t * inode_cont);
block_nr_t tfs_inode_get_block(tosco_fs_t* tfs, tfs_inode_t * inode, block_nr_t fbn);



// Isso só é necessário se o tam se setor for diferente do tamanho de
// bloco. A partir do setor sect_nr, lê o primeiro setor e , se ainda
// tiver dados pra ler, passa pro próxim setor
int tfs_read_block_from_disk(disk_t * d, block_nr_t bn, unsigned char
        block[BLOCK_SIZE]) {
    
    unsigned char * block_cursor;
    long long remaining_read = BLOCK_SIZE;
    // Qual setor corresponde a esse bloco bn?
    // bn = 3 , 3*4096 = 12288   ,    12288/512 = 24
    // Quer acessar o setor 24
    off_t sect_nr = bn * BLOCK_SIZE / SECTOR_SIZE;

    // Lê um setor na posição atual do cursor
    block_cursor = block;
    while (remaining_read > 0) {
        if (disk_read_sector(d, sect_nr, block_cursor) != 0) {
            // erro ao ler do disco
            return -1;
        }

        // Avança o cursor no buffer, avança o numero do setor,
        // decrementa o quanto falta ler, avança 
        block_cursor += SECTOR_SIZE;
        sect_nr++;
        remaining_read -= SECTOR_SIZE;
    }

    return 0;
}

// Isso só é necessário se o tam se setor for diferente do tamanho de
// bloco. A partir do setor sect_nr, escreve o primeiro setor e , se ainda
// tiver dados pra escrever, passa pro próximo setor
int tfs_write_block_to_disk(disk_t * d, block_nr_t bn, unsigned char
        block[BLOCK_SIZE]) {
    
    unsigned char * block_cursor;
    long long remaining_write = BLOCK_SIZE;
    // Qual setor corresponde a esse bloco bn?
    // bn = 3 , 3*4096 = 12288   ,    12288/512 = 24
    // Quer acessar o setor 24
    off_t sect_nr = bn * BLOCK_SIZE / SECTOR_SIZE;

    // escreve o setor na posição atual do cursor
    block_cursor = block;
    while (remaining_write > 0) {
        if (disk_write_sector(d, sect_nr, block_cursor) != 0) {
            // erro ao ler do disco
            return -1;
        }

        // Avança o cursor no buffer, avança o numero do setor,
        // decrementa o quanto falta ler, avança 
        block_cursor += SECTOR_SIZE;
        sect_nr++;
        remaining_write -= SECTOR_SIZE;
    }

    return 0;
}

int tfs_write_bitmap(disk_t * d, bitmap_t * bm) {
    // Escreve o bitmap a partir do bloco 1
    long bm_nblocks = bitmap_nchunks(bm, BLOCK_SIZE);
    unsigned char block[BLOCK_SIZE];

    // QUantos bytes faltam copiar?
    long long rem_bytes = bm->mem_size;
    // De onde estou copiando?
    unsigned char * bm_cursor = bm->bm_array;

    // Começa a gravar o bitmap no bloco 1
    block_nr_t block_cursor = 1;

    for (int i = 0; i < bm_nblocks; i++) {
        long nbytes = MIN(BLOCK_SIZE, rem_bytes);
        bzero(block, BLOCK_SIZE);
        memcpy(block, bm_cursor, nbytes);

        if (tfs_write_block_to_disk(d, block_cursor, block)) {
            printf("Erro gravando %d-ésimo bloco do bitmap\n", i);
            return -1;
        }

        bm_cursor += nbytes;
        block_cursor++;
        rem_bytes -= nbytes;
    }

    return 0;
}


/*---------------------------------------------------------------------------
 *  Function: tfs_mkfs
 *  Description: Cria um sistema de arquivos no disco especificado
 *  Returns: 0 on success, -1 on error
 *--------------------------------------------------------------------------*/
int tfs_mkfs(disk_t * d) {

    long long nblocks = d->size * SECTOR_SIZE / BLOCK_SIZE;
    unsigned char block[BLOCK_SIZE];

    // Precisa de pelo menos 3 blocos: superblock , bitmap e diretorio raiz
    // Com 3 blocos ou menos, não dá para fazer NADA
    if (nblocks <= 3) {
        printf("Disco muito pequeno\n");
        return -1;
    }

    bitmap_t * occupied_blocks = bitmap_new(nblocks);
    if (!occupied_blocks) {
        printf("Erro de memoria ao criar sistema de arquivox\n");
        return -1;
    }

    long long bm_nblocks = bitmap_nchunks(occupied_blocks, BLOCK_SIZE);

    block_nr_t sb_nblocks = nblocks;
    block_nr_t sb_block_raiz = 1 + bm_nblocks;

    // Cria o superblock
    bzero(block, BLOCK_SIZE);
    unsigned char * block_cursor = block;
    memcpy(block_cursor, &sb_nblocks, sizeof(block_nr_t));
    block_cursor += sizeof(block_nr_t);
    memcpy(block_cursor, &sb_block_raiz, sizeof(block_nr_t));
    block_cursor += sizeof(block_nr_t);

    if (tfs_write_block_to_disk(d, 0, block)) {
        printf("Erro gravando superblock\n");
        return -1;
    }

    // Agora cria o diretório raiz
    
    tfs_d_entry_t raiz;
    // Todos os nomes estão zerados, então não contém nada
    bzero(&raiz, sizeof(tfs_d_entry_t));

    memcpy(block, &raiz, sizeof(tfs_d_entry_t));
    if (tfs_write_block_to_disk(d, sb_block_raiz, block)) {
        printf("Erro gravando diretorio raiz\n");
        return -1;
    }

    // Agora atualiza o bitmap
    bitmap_set(occupied_blocks, 0, 1); // superblock está ocupado
    bitmap_set(occupied_blocks, sb_block_raiz, 1); // bloco com dir raiz está
                                                    // ocupado
    for (int i = 0; i < bm_nblocks; i++) {
        bitmap_set(occupied_blocks, i+1, 1); // Cada bloco ocupado pelo bitmap
                                             // também está ocupado!
    }

    if (tfs_write_bitmap(d, occupied_blocks)) {
        printf("MKFS: Erro gravando o bitmap\n");
        return -1;
    }

    printf("Criado ToscoFS com %lld blocos\n"
           "    Blocos de bitmap: %lld\n"
           "    Diretório raiz: %lld\n",
           sb_nblocks, bm_nblocks, sb_block_raiz);
    printf("ToscoFS: \n"
           "   N_ENTRIES_DENTRY: %lu\n", N_ENTRIES_DENTRY);
    printf("   N_ENTRIES_INODE: %lu\n ", N_ENTRIES_INODE);
    printf("       %ld bytes\n", (N_ENTRIES_INODE-1)*BLOCK_SIZE);
    printf("   N_ENTRIES_INODE_CONT: %lu\n", N_ENTRIES_INODE_CONT);
    printf("       %lu bytes\n", (N_ENTRIES_INODE_CONT-1)*BLOCK_SIZE);

    return 0;
}


/*---------------------------------------------------------------------------
 *  Function: tfs_mount
 *  Description: Inicializa um tosco-fs em cima de um arquivo de "disco"
 *--------------------------------------------------------------------------*/
tosco_fs_t * tfs_mount(disk_t * d) {

    // O nro de blocos é o tamanho total do disco dividido pelo tamanho do
    // bloco
    long long nblocks = d->size * SECTOR_SIZE / BLOCK_SIZE;
    unsigned char block[BLOCK_SIZE];

    // Zera arquivos abertos
    for(int i = 0; i < TFS_MAX_OPEN_FILES; i++)
    {
        tfs_opened_files[i] = NULL;
    }
   
    // Cria as coisas em memória
    // Novo SuperBlock em memória
    tosco_fs_t * tfs = malloc(sizeof(tosco_fs_t));
    if (!tfs) return NULL;
    
    tfs->disk = d;
   
    // Novo diretório raiz (d-entry) em memória
    tfs->dir = malloc(sizeof(tfs_d_entry_t));
    if (!tfs->dir) {
        free(tfs);
        return NULL;
    }

    // Novo bitmap de blocos livres em memória
    tfs->occupied_blocks = bitmap_new(nblocks);
    if (!tfs->occupied_blocks) {
        free(tfs->dir);
        free(tfs);
        return NULL;
    }

    // Tudo pronto, agora puxa do disco

    // Lê o superblock
    if (tfs_read_block_from_disk(d, 0, block) != 0) {
        // erro
        free(tfs->dir);
        free(tfs);
        return NULL;
    }

    // Dentro do superblock, quando eu fiz mkfs, eu coloquei:
    //  nr de blocos qd eu formatei
    //  nr do bloco que contem o dir raiz
    
    block_nr_t sb_nblocks;
    block_nr_t sb_block_raiz;

    // Copia do bloco lido para as variáveis
    unsigned char * block_cursor = block;
    memcpy(&sb_nblocks, block_cursor, sizeof(sb_nblocks));
    block_cursor += sizeof(sb_nblocks);
    memcpy(&sb_block_raiz, block_cursor, sizeof(sb_block_raiz));

    if (sb_nblocks != nblocks) {
        // Ooops, esse disco não tá muito certo não
        printf("ERRO lendo superblock!: esperava %lld blocos, tem %lld\n",
                nblocks, sb_nblocks);
        free(tfs->dir);
        free(tfs->occupied_blocks);
        free(tfs);
        return NULL;
    }

    // Agora lê o bitmap para a memória
    // O bitmap sempre começa no bloco 1
    // Começando com um cursor de bloco em 1 (bitmap_block), e um cursor de
    // memória do bitmap no início do array (bm_cursor), lê um bloco inteiro
    // em memória (block). Copia uma quantidade de bytes para o bitmap.
    // A cópia para o bitmap OU VAI SER O BLOCO INTEIRO OU VAI SER O QUE FALTA
    //
    // Se o bitmap ocupa 9000 bytes, 
    // Lê um bloco de 4096 , copia tudo começando na posição 0 do bitmap
    //      Resta 4904
    // Lê um bloco de 4096 , copia tudo começando na posição 4096 do bitmap
    //      Resta 808
    // Lê um bloco de 4096, copia 808 bytes para a posição 8192 do bitmap
    // R
    unsigned char * bm_cursor = tfs->occupied_blocks->bm_array;
    long long bitmap_block = 1;
    long long bitmap_nbytes = tfs->occupied_blocks->mem_size;
    while (bitmap_nbytes > 0) {
        if (tfs_read_block_from_disk(d, bitmap_block, block) != 0) {
            // erro
            printf("ERRO lendo bitmap!\n");
            free(tfs->dir);
            free(tfs->occupied_blocks);
            free(tfs);
            return NULL;
        }

        long long rem_bytes = MIN(BLOCK_SIZE, bitmap_nbytes);
        memcpy(bm_cursor, block, rem_bytes);
        printf("Leu 1 bloco de bitmap, %lld bytes\n", rem_bytes);

        bm_cursor += BLOCK_SIZE;
        bitmap_block++;
        bitmap_nbytes -= rem_bytes;
    }

    // Bitmap em memória
    // Se o bloco 0 estiver "livre" (não setado), é um erro!
    int occupied_sb = bitmap_get(tfs->occupied_blocks, 0);
    if (!occupied_sb) {
        printf("Erro! Bitmap me diz que superblock está livre!");
        free(tfs->dir);
        free(tfs->occupied_blocks);
        free(tfs);
        return NULL;
    } else {
        printf("Superbloco ocupado: %d\n", occupied_sb);
    }

    // Nro de blocos no bitmap: Se o tamanho em memória é **exato** múltiplo
    // de BLOCK_SIZE, o bitmap ocupa mem_size / BLOCKSIZE
    // Senão, ele ocupa 1 + mem_size / BLOCK_SIZE
    // // Isso é o "arredonda para cima"
    long long bm_nblocks = bitmap_nchunks(tfs->occupied_blocks, BLOCK_SIZE);

    for (int i = 0; i < bm_nblocks; i++) {
        // Se o bloco 1 + i estiver livre, é um erro
        // esses blocos estão ocupados pelo bitmap!
        if (bitmap_get(tfs->occupied_blocks, 1 + i) <= 0) 
        {
            printf("Erro! Bitmap me diz que bloco %d está livreou com erro!", i+1);
            free(tfs->dir);
            free(tfs->occupied_blocks);
            free(tfs);
            return NULL;
        }
    }

    // Agora lê o diretório raiz
    if (tfs_read_dentry(tfs, sb_block_raiz, tfs->dir) < 0) {
        printf("Não pôde ler o diretório raiz!\n");
        free(tfs->dir);
        free(tfs->occupied_blocks);
        free(tfs);
        return NULL;
    }

    tfs->sb_bloco_raiz = sb_block_raiz;
    tfs->sb_nblocks = sb_nblocks;
    
    printf("Montado ToscoFS com %lld blocos\n"
           "    Blocos de bitmap: %lld\n"
           "    Diretório raiz: %lld\n",
           tfs->sb_nblocks, bm_nblocks, tfs->sb_bloco_raiz);

    // Showtime!
    return tfs;
}


/*---------------------------------------------------------------------------
 *  Function: tfs_get_free_block
 *  Description: Pega um bloco livre e o marca como ocupado
 *  Returns: -1 caso não tenha mais blocos livres, cc o número do primeiro
 *  bloco livre
 *--------------------------------------------------------------------------*/
block_nr_t tfs_get_free_block(tosco_fs_t * tfs) {
    // encontra o primeiro não ocupado
    block_nr_t ret = bitmap_find_first_unset(tfs->occupied_blocks);
    if (ret > 0) {
        bitmap_set(tfs->occupied_blocks, ret, 1);
        tfs_write_bitmap(tfs->disk, tfs->occupied_blocks);

        // Zero que block on disk, just in case
        // Simplifies handling of newly allocated blocks
        unsigned char block[BLOCK_SIZE];
        bzero(block, BLOCK_SIZE);
        tfs_write_block_to_disk(tfs->disk, ret, block);

        //printf("DEBUG: get_free_block -> %lld\n", ret);
    } else {
        //printf("DEBUG: tfs_get_free_block -> NOPS\n");
    }

    return ret;
}

/*---------------------------------------------------------------------------
 *  Function: tfs_put_block
 *  Description: Libera um bloco
 *  Returns: -1 caso não tenha mais blocos livres, cc o número do primeiro
 *  bloco livre
 *--------------------------------------------------------------------------*/
int tfs_put_block(tosco_fs_t * tfs, block_nr_t bn) {
    // encontra o primeiro não ocupado
    if (!bitmap_get(tfs->occupied_blocks, bn)) {
        // Ué, não tava ocupado!
        printf("Liberando bloco livre!\n");
        return -1;
    }

    if (bitmap_set(tfs->occupied_blocks, bn, 0) < 0) {
        printf("Ooops, não tenho bloco %lld para liberar\n", bn);
        return -1;
    }

    tfs_write_bitmap(tfs->disk, tfs->occupied_blocks);

    return 0;
}


/*===========================================================================
 * Funções de d-entry:
 * - escrever a dentry : tfs_write_dentry
 * - ler uma dentry do disco: tfs_read_dentry
 * - buscar um nome dentro da dentry: tfs_dentry_find_entry
 * - adicionar uma entrada na dentry: tfs_dentry_add_entry
 * - remover uma entrada da dentry: tfs_dentry_del_entry
 *=========================================================================*/


/*---------------------------------------------------------------------------
 *  Function: tfs_write_dentry
 *  Description: Escreve uma dentry para um bloco
 *  Return: 0 em sucesso, -1 caso erro
 *--------------------------------------------------------------------------*/
int tfs_write_dentry(tosco_fs_t * tfs, block_nr_t bn, tfs_d_entry_t * dentry)
{
    unsigned char block[BLOCK_SIZE];
    bzero(block, BLOCK_SIZE);
    memcpy(block, dentry->entries, sizeof(dentry->entries));

    if (tfs_write_block_to_disk(tfs->disk, bn, block)) {
        printf("Erro gravando dentry no bloco %lld\n", bn);
        return -1;
    }

    return 0;
}


/*---------------------------------------------------------------------------
 *  Function: tfs_read_dentry
 *  Description: Lê uma dentry de um bloco
 *--------------------------------------------------------------------------*/
int tfs_read_dentry(tosco_fs_t * tfs, block_nr_t bn, tfs_d_entry_t * dentry) {
    unsigned char block[BLOCK_SIZE];
    bzero(block, BLOCK_SIZE);

    if (tfs_read_block_from_disk(tfs->disk, bn, block)) {
        printf("Erro lendo dentry no bloco %lld\n", bn);
        return -1;
    }
    memcpy(dentry->entries, block, sizeof(dentry->entries));

    return 0;
}

/*---------------------------------------------------------------------------
 *  Function: tfs_dentry_find_entry
 *  Description: Procura um nome no diretório e retorna o bloco onde tem o
 *               inode
 *  Return: Bloco onde está o inode; -1 caso não encontrado
 *--------------------------------------------------------------------------*/
block_nr_t tfs_dentry_find_entry(tfs_d_entry_t * dentry,
                                 const char * name,
                                 int *pos) {
    for (int i = 0; i < N_ENTRIES_DENTRY; i++) {
        if (!strncmp(dentry->entries[i].name, name, TFS_MAX_NAME_LEN)) {
            if (pos)
                *pos = i;
            return dentry->entries[i].inode_pos;
        }
    }

    return -1;
}

/*---------------------------------------------------------------------------
 *  Function: tfs_dentry_add_entry
 *  Description: Adiciona uma entrada a uma dentry
 *  Returns: 0 caso sucesso, -1 caso erro
 *--------------------------------------------------------------------------*/
int tfs_dentry_add_entry(tfs_d_entry_t * dentry,
                         const char * name,
                         block_nr_t bn)
{
    if (tfs_dentry_find_entry(dentry, name, NULL) > 0) {
        // Ooops, esse nome já existe!
        return -1;
    }

    for (int i = 0; i < N_ENTRIES_DENTRY; i++) {
        if (dentry->entries[i].name[0] == 0) {
            //Posição vazia   !
            // COloca aqui. Copia o nome (máximo 32 caracteres)
            strncpy(dentry->entries[i].name, name, TFS_MAX_NAME_LEN);
            dentry->entries[i].name[TFS_MAX_NAME_LEN] = 0;
            dentry->entries[i].inode_pos = bn;
            return 0;
        }
    }

    // Ooops, cheio!
    return -1;
}

/*---------------------------------------------------------------------------
 *  Function: tfs_dentry_del_entry
 *  Description: Remove uma entrada a uma dentry
 *  Returns: 0 caso sucesso, -1 caso erro
 *--------------------------------------------------------------------------*/
int tfs_dentry_del_entry(tfs_d_entry_t * dentry, int pos) {
    if (pos >= N_ENTRIES_DENTRY || 
            dentry->entries[pos].name[0] == 0) {
        // Posição inválida ou essa entrada nem estava ocupada!
        return -1;
    }

    dentry->entries[pos].name[0] = 0;
    dentry->entries[pos].inode_pos = 0;

    return 0;
}



/*---------------------------------------------------------------------------
 *  Function: tfs_read_inode
 *  Description: Lê um inode do disco
 *--------------------------------------------------------------------------*/
int tfs_read_inode(tosco_fs_t * tfs, block_nr_t bn, tfs_inode_t * inode)
{
    unsigned char block[BLOCK_SIZE];
    bzero(block, BLOCK_SIZE);

    if (tfs_read_block_from_disk(tfs->disk, bn, block)) {
        printf("Erro lendo inode\n");
        return -1;
    }

    inode->where = bn;
    unsigned char * cursor = block;
    memcpy(&inode->attrs.tamanho, cursor, sizeof(tfs_attr_t));
    cursor += sizeof(tfs_attr_t);
    memcpy(inode->mapping, cursor, sizeof(inode->mapping));

    return 0;
}

/*---------------------------------------------------------------------------
 *  Function: tfs_write_inode
 *  Description: escreve um inode do disco pra onde ele estava
 *--------------------------------------------------------------------------*/
int tfs_write_inode(tosco_fs_t * tfs, tfs_inode_t * inode)
{
    unsigned char block[BLOCK_SIZE];
    bzero(block, BLOCK_SIZE);

    unsigned char * cursor = block;
    memcpy(cursor, &inode->attrs.tamanho, sizeof(tfs_attr_t));
    cursor += sizeof(tfs_attr_t);
    memcpy(cursor, inode->mapping, sizeof(inode->mapping));
    if (tfs_write_block_to_disk(tfs->disk, inode->where, block)) {
        printf("Erro escrevendo inode\n");
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------
 *  Function: tfs_read_inode_cont
 *  Description: Lê um inode de continuação
 *--------------------------------------------------------------------------*/
int tfs_read_inode_cont(tosco_fs_t * tfs,
                        block_nr_t bn,
                        tfs_inode_cont_t * inode_cont)
{
    unsigned char block[BLOCK_SIZE];
    bzero(block, BLOCK_SIZE);

    if (tfs_read_block_from_disk(tfs->disk, bn, block)) {
        printf("Erro lendo inode\n");
        return -1;
    }

    memcpy(inode_cont->mapping, block, sizeof(inode_cont->mapping));
    inode_cont->where = bn;

    return 0;
}

/*---------------------------------------------------------------------------
 *  Function: tfs_write_inode_cont
 *  Description: Lê um inode de continuação
 *--------------------------------------------------------------------------*/
int tfs_write_inode_cont(tosco_fs_t * tfs,
                        tfs_inode_cont_t * inode_cont)
{
    unsigned char block[BLOCK_SIZE];
    bzero(block, BLOCK_SIZE);
    memcpy(block, inode_cont->mapping, sizeof(inode_cont->mapping));

    if (tfs_write_block_to_disk(tfs->disk, inode_cont->where, block)) {
        printf("Erro escrevendo inode\n");
        return -1;
    }

    return 0;
}


/*---------------------------------------------------------------------------
 *  Function: tfs_inode_get_block
 *  Description: Retorna onde no disco está um bloco específico do arquivo
 *--------------------------------------------------------------------------*/
block_nr_t tfs_inode_get_block(tosco_fs_t* tfs, tfs_inode_t * inode, block_nr_t fbn)
{
    if (fbn < N_ENTRIES_INODE-1) {
        // Esse mapeamento está dentro do inode
        if (inode->mapping[fbn] <= 0) {
            // Hmmm, ainda não alocado!
            block_nr_t new_block = tfs_get_free_block(tfs);
            if (new_block <= tfs->sb_bloco_raiz) {
                printf("Ganhou bloco %lld, anterior a %lld\n",
                        new_block, tfs->sb_bloco_raiz);
                return -1;
            }
            inode->mapping[fbn] = new_block;
            if (tfs_write_inode(tfs, inode)) {
                printf("Erro atualizando inode no disco\n");
                return -1;
            }
        }

        return inode->mapping[fbn];
    } else {
        // Tem que procurar nos inodes de continuação!
        // Vai começar a ver a continuação, que começa no bloco
        // N_ENTRIES_INODE -- Se temos 10 entradas, os blocos 0 a 9 estão no
        // inode. O bloco 10 está na primeira continuação (10-19) ...
        tfs_inode_cont_t cont;
        fbn -= (N_ENTRIES_INODE - 1);
        block_nr_t cont_bn = inode->mapping[N_ENTRIES_INODE-1];

        // Garante que essa continuação existe
        if (cont_bn <= 0) {
            // Essa continuação ainda não foi alocada
            cont_bn = tfs_get_free_block(tfs);
            if (cont_bn <= tfs->sb_bloco_raiz) {
                printf("Sem blocos livres para continuação\n");
                return -1;
            }
            // Atualiza no disco
            cont.where = cont_bn;
            bzero(cont.mapping, sizeof(cont.mapping));
            if (tfs_write_inode_cont(tfs, &cont))
                return -1;

            inode->mapping[N_ENTRIES_INODE - 1] = cont_bn;
            if (tfs_write_inode(tfs, inode))
                return -1;
        } else {
            if (tfs_read_inode_cont(tfs, cont_bn, &cont))
                return -1;
        }

        // Procura na continuação atual
        do {
            if (fbn < N_ENTRIES_INODE_CONT - 1)
            {
                // O bloco está nesta continuação!
                if (cont.mapping[fbn] <= 0) {
                    // Hmmm, ainda não alocado!
                    block_nr_t new_block = tfs_get_free_block(tfs);
                    if (new_block <= tfs->sb_bloco_raiz) {
                        printf("Erro! Ganhou bloco %lld, anterior a %lld\n",
                                new_block, tfs->sb_bloco_raiz);
                        return -1;
                    }
                    cont.mapping[fbn] = new_block;
                    if (tfs_write_inode_cont(tfs, &cont)) {
                        printf("Erro atualizando inode no disco\n");
                        return -1;
                    }
                }

                return cont.mapping[fbn];
            } else {
                // Tem que olhar para a próxima continuação
                
                fbn -= (N_ENTRIES_INODE_CONT - 1);
                cont_bn = cont.mapping[N_ENTRIES_INODE_CONT-1];

                // Garante que essa continuação existe
                if (cont_bn <= 0) {
                    // Essa continuação ainda não foi alocada
                    cont_bn = tfs_get_free_block(tfs);
                    if (cont_bn <= tfs->sb_bloco_raiz) {
                        printf("Erro! Ganhou bloco %lld, anterior a %lld\n",
                                cont_bn, tfs->sb_bloco_raiz);
                        printf("Sem blocos livres para continuação\n");
                        return -1;
                    }
                    tfs_inode_cont_t new_cont;
                    // Atualiza no disco
                    new_cont.where = cont_bn;
                    bzero(new_cont.mapping, sizeof(new_cont.mapping));
                    tfs_write_inode_cont(tfs, &new_cont);

                    // Atualiza a continuação atual
                    cont.mapping[N_ENTRIES_INODE_CONT - 1] = cont_bn;
                    tfs_write_inode_cont(tfs, &cont);
                    
                    // Avança para a próxima continuação (recém criada)
                    cont = new_cont;
                } else {
                    // Lê a continuação do disco
                    tfs_read_inode_cont(tfs, cont_bn, &cont);
                }
            }
        } while (fbn >= 0);
    }

    // Nunca deve chegar aqui, mas se chegou é erro!
    return -1;
}

int tfs_inode_free(tosco_fs_t * tfs, block_nr_t bn_inode)
{
    tfs_inode_t inode;
    if (tfs_read_inode(tfs, bn_inode, &inode)) {
        printf("Erro lendo inode\n");
        return -1;
    }
    
    // Libera todas as entradas diretas
    for (int i = 0; i < N_ENTRIES_INODE-1; i++) {
        if (inode.mapping[i] > 0) {
            tfs_put_block(tfs, inode.mapping[i]);
        }
    }

    if (inode.mapping[N_ENTRIES_INODE - 1]) {
        // Libera os blocos de todas as continuações
        tfs_inode_cont_t cont;
        block_nr_t cont_bn = inode.mapping[N_ENTRIES_INODE - 1];
        do {
            if (tfs_read_inode_cont(tfs, cont_bn, &cont))
            {
                printf("Erro lendo inode cont\n");
                return -1;
            }

            for (int i = 0; i < N_ENTRIES_INODE_CONT - 1; i++) {
                if (cont.mapping[i] > 0)
                    tfs_put_block(tfs, cont.mapping[i]);
            }

            // Próximo bn é aqui
            cont_bn = cont.mapping[N_ENTRIES_INODE_CONT - 1];

            // Libera esta continuacao
            tfs_put_block(tfs, cont.where);
        } while (cont_bn > 0);

    }

    tfs_put_block(tfs, inode.where);

    return 0;
}


void tfs_info(tosco_fs_t * tfs) {
    long long nblocks = 0;
    printf("ToscoFS: \n"
           "   N_ENTRIES_DENTRY: %lu\n", N_ENTRIES_DENTRY);
    printf("   N_ENTRIES_INODE: %lu\n ", N_ENTRIES_INODE);
    printf("       %ld bytes\n", (N_ENTRIES_INODE-1)*BLOCK_SIZE);
    printf("   N_ENTRIES_INODE_CONT: %lu\n", N_ENTRIES_INODE_CONT);
    printf("       %lu bytes\n", (N_ENTRIES_INODE_CONT-1)*BLOCK_SIZE);

    for (int i = 0; i < tfs->occupied_blocks->size; i++) {
        if (bitmap_get(tfs->occupied_blocks, i))
            nblocks++;
    }

    printf("Blocks: %lld / %lld (occupied/total)\n",
            nblocks, tfs->occupied_blocks->size);
}

void tfs_inode_info(tosco_fs_t * tfs, tfs_inode_t  * inode)
{
    printf("Inode ocupa bloco %lld\n", inode->where);
    for (int i = 0; i < N_ENTRIES_INODE - 1; i++) {
        if (inode->mapping[i] > 0)
            printf("Dados do bloco %d em %lld\n", i, inode->mapping[i]);
    }

    if (inode->mapping[N_ENTRIES_INODE - 1]) {
        tfs_inode_cont_t cont;
        block_nr_t cont_bn = inode->mapping[N_ENTRIES_INODE - 1];

        do {
            tfs_read_inode_cont(tfs, cont_bn, &cont);
            printf("Continuação em %lld == %lld\n", cont_bn, cont.where);
            for (int i = 0; i < N_ENTRIES_INODE_CONT - 1; i++) {
                if (cont.mapping[i] > 0)
                    printf("Dados do bloco %d em %lld\n", i, cont.mapping[i]);
            }

            cont_bn = cont.mapping[N_ENTRIES_INODE_CONT - 1];
        } while (cont_bn);
    }
}

/*****************************************************************************
 * Funções de sistema de arquivos de fato
 * tfs_create
 * tfs_delete
 * tfs_open
 * tfs_write
 * tfs_read
 * tfs_close
 *****************************************************************************/



int tfs_create(tosco_fs_t * tfs, const char * name)
{
    if (tfs_dentry_find_entry(tfs->dir, name, NULL) > 0) {
        printf("Arquivo já existe\n");
        return -1;
    }

    tfs_inode_t inode;
    bzero(&inode, sizeof(inode));
    inode.where = tfs_get_free_block(tfs);

    if (inode.where <= tfs->sb_bloco_raiz) {
        // Hmmm, sem espaço livre!
        printf("create: Sem espaço livre: ganhou %lld\n", inode.where);
        return -1;
    }

    // Guarda o inode, completamente zerado
    // O mapeamento leva sempre pra zero (que significa indefinido), o tamanho
    // é zero
    tfs_write_inode(tfs, &inode);

    if (tfs_dentry_add_entry(tfs->dir, name, inode.where) < 0){
        // Não conseguiu criar o arquivo
        // Esse inode não é mais necessário
        printf("Erro adicionando à entrada\n");
        tfs_put_block(tfs, inode.where);
        return -1;
    }

    // Guarda no disco o diretório atualizado
    tfs_write_dentry(tfs, tfs->sb_bloco_raiz, tfs->dir);

    return 0;
}

int tfs_delete(tosco_fs_t * tfs, const char * fname) {
    int pos;
    block_nr_t b_inode = tfs_dentry_find_entry(tfs->dir, fname, &pos);
    if (b_inode < 0) {
        // not found
        return -1;
    }

    // apaga a entrada do diretorio
    if (tfs_dentry_del_entry(tfs->dir, pos) < 0) {
        printf("Erro ao apagar entrada do diretorio\n");
        return -1;
    }

    printf("vai liberar inode %lld\n", b_inode);
    if (tfs_inode_free(tfs, b_inode) < 0) {
        printf("Erro ao liberar os inodes do arquivo %s\n", fname);
    }
    
    // Guarda no disco o diretório atualizado
    tfs_write_dentry(tfs, tfs->sb_bloco_raiz, tfs->dir);

    return 0;

}

int tfs_get_size_and_usage(tosco_fs_t * tfs,
                            block_nr_t bn,
                            tfs_size_t * size_out,
                            tfs_size_t * blocks_out)
{
    tfs_inode_t inode;
    *size_out = 0;
    *blocks_out = 0;
    if (tfs_read_inode(tfs, bn, &inode)) 
        return -1;

    *size_out = inode.attrs.tamanho;
    for (int i = 0; i < N_ENTRIES_INODE - 1; i++)
    {
        if (inode.mapping[i] > 0)
            (*blocks_out)++;
    }

    if (inode.mapping[N_ENTRIES_INODE - 1]) {
        tfs_inode_cont_t cont;
        block_nr_t cont_bn = inode.mapping[N_ENTRIES_INODE - 1];

        do {
            if (tfs_read_inode_cont(tfs, cont_bn, &cont)) {
                return -1;
            }
            for (int i = 0; i < N_ENTRIES_INODE_CONT - 1; i++) {
                if (cont.mapping[i] > 0) {
                    (*blocks_out)++;
                }
            }

            cont_bn = cont.mapping[N_ENTRIES_INODE_CONT - 1];

        } while (cont_bn);
    }

    return 0;
}

int tfs_list_dir(tosco_fs_t *tfs, struct tfs_lsdir_entry * out, int max)
{
    int nentries = 0;

    for (int i = 0; i < N_ENTRIES_DENTRY; i++) {
        // Pára caso atinga max entradas
        if (nentries >= max)
            break;

        if (tfs->dir->entries[i].name[0]) {
            // Entrada não vazia
            tfs_inode_t inode;

            // Copia o nome
            strncpy(out[nentries].name, tfs->dir->entries[i].name, TFS_MAX_NAME_LEN);

            // Pega o tamanho do arquivo e a quantidade de blocos ocupados
            if (tfs_get_size_and_usage(tfs, tfs->dir->entries[i].inode_pos,
                                        &(out[nentries].size),
                                        &(out[nentries].blocks_used)))
            {
                return -1;
            }

            nentries++;
        }
    }

    return nentries;
}


int tfs_open_file(tosco_fs_t * tfs, const char * fname)
{
    int fd = -1;
    for (int i = 0; i < TFS_MAX_OPEN_FILES; i++)
        if (tfs_opened_files[i] == NULL) {
            fd = i;
            break;
        }

    block_nr_t b_inode = tfs_dentry_find_entry(tfs->dir, fname, NULL);

    if (b_inode < 0) return -1;

    tfs_opened_files[fd] = malloc(sizeof(tfs_inode_t));
    if (!tfs_opened_files[fd]) 
        return -1;

    if (tfs_read_inode(tfs, b_inode, tfs_opened_files[fd])) {
        free(tfs_opened_files[fd]);
        return -1;
    }

    return fd;
}

int tfs_close_file(tosco_fs_t * tfs, int fd) {
    if (tfs_opened_files[fd] == NULL)
        return -1;

    free(tfs_opened_files[fd]);

    tfs_opened_files[fd] = NULL;

    return 0;
}

int tfs_write(tosco_fs_t * tfs, int fd,
        long long offset, long long len, const char * buffer)
{
    unsigned char block[BLOCK_SIZE];
    block_nr_t start_bn = offset / BLOCK_SIZE;
    block_nr_t end_bn = (offset + len) / BLOCK_SIZE;
    long long offset_cursor = offset;
    long long rem_bytes = len;

    long long total_written = 0;
    
    if (tfs_opened_files[fd] == NULL)
        return -1;

    tfs_inode_t * inode = tfs_opened_files[fd];

    /*
     * A block across boundaries! \o/
     *  bottom    middle   top
     * +--------+--------+--------+
     * |    ****|********|***     |
     * +--------+--------+--------+
     *  start_bn           end_bn
     * */

    // Handles bottom part
    while (rem_bytes > 0) 
    {
        block_nr_t bn = tfs_inode_get_block(tfs, inode, start_bn);
        tfs_read_block_from_disk(tfs->disk, bn, block);

        // Copia buffer pra dentro do bloco, na posição correta
        int off_block = (offset_cursor % BLOCK_SIZE);
        int nbytes = MIN(BLOCK_SIZE-off_block, rem_bytes);
        memcpy(&(block[off_block]), buffer, nbytes);
        tfs_write_block_to_disk(tfs->disk, bn, block);

        // Avança para midle, atualiza quantos faltam para atualizar
        start_bn++;
        rem_bytes -= nbytes;
        offset_cursor += nbytes;
        buffer += nbytes;

        total_written += nbytes;
    }

    // Atualiza tamanho do arquivo
    
    inode->attrs.tamanho = MAX(inode->attrs.tamanho, offset + len);
    tfs_write_inode(tfs, inode);

    return total_written;
}

void tfs_file_info(tosco_fs_t * tfs, int fd) {
    tfs_inode_info(tfs, tfs_opened_files[fd]);
}

int tfs_read(tosco_fs_t * tfs, int fd,
        long long offset, long long len, char * buffer)
{
    unsigned char block[BLOCK_SIZE];
    block_nr_t start_bn = offset / BLOCK_SIZE;
    block_nr_t end_bn = (offset + len) / BLOCK_SIZE;
    long long offset_cursor = offset;
    long long rem_bytes = len;
    
    if (tfs_opened_files[fd] == NULL)
        return -1;

    tfs_inode_t * inode = tfs_opened_files[fd];

    long long total_read = 0;

    /*
     * A block across boundaries! \o/
     *  bottom    middle   top
     * +--------+--------+--------+
     * |    ****|********|***     |
     * +--------+--------+--------+
     *  start_bn           end_bn
     * */

    bzero(buffer, len);

    while (rem_bytes > 0) 
    {
        int off_block = (offset_cursor % BLOCK_SIZE);
        int nbytes = MIN(BLOCK_SIZE-off_block, rem_bytes);

        if (offset_cursor > inode->attrs.tamanho) {
            // Estou lendo além do fim do arquivo,
            break;
        }
        block_nr_t bn = tfs_inode_get_block(tfs, inode, start_bn);
        tfs_read_block_from_disk(tfs->disk, bn, block);

        // Copia do bloco para dentro do buffer
        memcpy(buffer, &(block[off_block]), nbytes);

        long long this_read = MIN(inode->attrs.tamanho - offset_cursor, nbytes);
        total_read += this_read;
        if (this_read < nbytes) {
            // Ooops, chegamos ao fim do arquivo
            break;
        }

        // Avança para midle, atualiza quantos faltam para atualizar
        start_bn++;
        offset_cursor += nbytes;
        rem_bytes -= nbytes;
        buffer += nbytes;

    }

    return total_read;
}
//int tfs_write_data(tosco_fs_t * tfs, 
