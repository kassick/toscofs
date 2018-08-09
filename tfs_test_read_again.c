/*
 * ===========================================================================
 *
 *       Filename:  tfs_create_files.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-11-2015 16:47:49
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
#include "tosco_fs.h"


#define MSG1 "isso eh um teste bla bla bla"
#define MSG2 "isso eh outro teste que nao eh o mesmo teste de antes"
#define MSG3 "essa mensagem eh uma feita para ser grande o suficiente para ficar no fim do bloco e passar do bloco para o proximo e parecer bem grande dentro do arquivo apesar de ser apenas baboseira e nao dizer nada com nada eh tipo um ipsis lorem imsum Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

int main(int argc, char *argv[])
{
    // NOvo disco de 8MB
    disk_t * disk = disk_new("teste1.img", (8*1000*1000)/SECTOR_SIZE);
    //disk_t * disk = disk_new("/tmp/teste1.img", (8*1000*1000)/SECTOR_SIZE);
    //disk_t * disk = disk_new("/dev/sdc1", (8*1000*1000)/SECTOR_SIZE);

    // Monta sem criar o sistema
    tosco_fs_t * tfs = tfs_mount(disk);
    if (!tfs) {
        printf("Não conseguiu montar o sistema!!!\n");
        exit(1);
    }

    struct tfs_lsdir_entry dir[200];
    int nentries = tfs_list_dir(tfs, dir, 200);
    for (int i = 0; i < nentries; i++) {
        printf("Arquivo: %s | Tam: %lld | Blocos ocupados: %lld\n",
                dir[i].name, dir[i].size, dir[i].blocks_used);
    }

    tfs_info(tfs);

    // Teste 1 foi apagado, usa teste5 no qual fizemos todos os mesmos testes
    int fd = tfs_open_file(tfs, "teste5");
    if (fd < 0) {
        printf("Ooops, não abriu arquivo\n");
        exit(1);
    }

    printf("File info de teste1\n");
    tfs_file_info(tfs, fd);

    tfs_write(tfs, fd, 1000 + strlen(MSG2), strlen(MSG1), MSG1);
    tfs_write(tfs, fd, 1000               , strlen(MSG2), MSG2);
    printf("----------------------------------\n");
    printf("TFS INFO DEPOIS DE ESCREVER:\n");
    tfs_info(tfs);
    printf("----------------------------------\n");

    nentries = tfs_list_dir(tfs, dir, 200);
    for (int i = 0; i < nentries; i++) {
        printf("Arquivo: %s | Tam: %lld | Blocos ocupados: %lld\n",
                dir[i].name, dir[i].size, dir[i].blocks_used);
    }

    char readbuf[2000];
    tfs_read(tfs, fd, 0, 2000, readbuf);
    readbuf[1999] = 0;

    // Isso deve imprimir MSG2 contatenada com MSG1
    printf("%s\n", &readbuf[10]);
    
    tfs_read(tfs, fd, 2088950, 2000, readbuf);
    readbuf[1999] = 0;
    
    // Isso deve imprimir MSG3
    printf("%s\n", readbuf);

    printf("----------------------------------\n");
    tfs_file_info(tfs, fd);
    tfs_close_file(tfs, fd);

    nentries = tfs_list_dir(tfs, dir, 200);
    for (int i = 0; i < nentries; i++) {
        printf("Arquivo: %s | Tam: %lld | Blocos ocupados: %lld\n",
                dir[i].name, dir[i].size, dir[i].blocks_used);
    }

    tfs_info(tfs);

    return 0;
}
