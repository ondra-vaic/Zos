//
// Created by me on 1/10/20.
//

#ifndef ZOS_SUPERBLOCK_H
#define ZOS_SUPERBLOCK_H


#include <stdint-gcc.h>
#include <fstream>
using namespace std;

struct SuperBlock{
    char signature[9];                      //login autora FS
    char volume_descriptor[251];            //popis vygenerovaného FS
    int32_t disk_size;                      //celkova velikost VFS
    int32_t cluster_size;                   //velikost clusteru
    int32_t cluster_count;                  //pocet clusteru
    int32_t inode_count;                    //pocet clusteru
    int32_t cluster_bitmap_start_address;   //adresa pocatku bitmapy datových bloků
    int32_t inode_bitmap_start_address;     //adresa pocatku bitmapy datových bloků
    int32_t inode_start_address;            //adresa pocatku  i-uzlů
    int32_t data_start_address;             //adresa pocatku datovych bloku

    SuperBlock() = default;
    SuperBlock(const string& fileSystemName, int32_t offset);
    SuperBlock(char signature[9], char volume_descriptor[251], int32_t disk_size, int32_t cluster_size,
               int32_t cluster_count, int32_t inode_count, int32_t cluster_bitmap_start_address,
               int32_t inode_bitmap_start_address, int32_t inode_start_address, int32_t data_start_address);

    void Save(const string& fileSystemName, int32_t offset);
};


#endif //ZOS_SUPERBLOCK_H
