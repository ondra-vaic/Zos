//
// Created by me on 1/10/20.
//


#include <cstring>
#include "SuperBlock.h"


void SuperBlock::Save(const string& fileSystemName, int32_t offset) {

    fstream stream(fileSystemName, std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(offset, ios_base::beg);

    stream.write((char*)&signature, sizeof(signature));
    stream.write((char*)&volume_descriptor, sizeof(volume_descriptor));
    stream.write((char*)&disk_size, sizeof(disk_size));
    stream.write((char*)&cluster_size, sizeof(cluster_size));
    stream.write((char*)&cluster_count, sizeof(cluster_count));
    stream.write((char*)&inode_count, sizeof(inode_count));
    stream.write((char*)&cluster_bitmap_start_address, sizeof(cluster_bitmap_start_address));
    stream.write((char*)&inode_bitmap_start_address, sizeof(inode_bitmap_start_address));
    stream.write((char*)&inode_start_address, sizeof(inode_start_address));
    stream.write((char*)&data_start_address, sizeof(data_start_address));

    stream.close();
}

SuperBlock::SuperBlock(const string& fileSystemName, int32_t offset){

    ifstream stream(fileSystemName, std::ios::binary);
    stream.seekg(offset, ios_base::beg);

    stream.read((char*)&signature, sizeof(signature));
    stream.read((char*)&volume_descriptor, sizeof(volume_descriptor));
    stream.read((char*)&disk_size, sizeof(disk_size));
    stream.read((char*)&cluster_size, sizeof(cluster_size));
    stream.read((char*)&cluster_count, sizeof(cluster_count));
    stream.read((char*)&inode_count, sizeof(inode_count));
    stream.read((char*)&cluster_bitmap_start_address, sizeof(cluster_bitmap_start_address));
    stream.read((char*)&inode_bitmap_start_address, sizeof(inode_bitmap_start_address));
    stream.read((char*)&inode_start_address, sizeof(inode_start_address));
    stream.read((char*)&data_start_address, sizeof(data_start_address));

    stream.close();
}

SuperBlock::SuperBlock(char signature[9], char volume_descriptor[251], int32_t disk_size, int32_t cluster_size,
                       int32_t cluster_count, int32_t inode_count, int32_t cluster_bitmap_start_address,
                       int32_t inode_bitmap_start_address, int32_t inode_start_address, int32_t data_start_address){

    strcpy(this->signature, signature);
    strcpy(this->volume_descriptor, volume_descriptor);

    this->disk_size = disk_size;
    this->cluster_size = cluster_size;
    this->cluster_count = cluster_count;
    this->inode_count = inode_count;
    this->cluster_bitmap_start_address = cluster_bitmap_start_address;
    this->inode_bitmap_start_address = inode_bitmap_start_address;
    this->inode_start_address = inode_start_address;
    this->data_start_address = data_start_address;
}
