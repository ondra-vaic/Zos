//
// Created by me on 1/10/20.
//


#include <iostream>
#include <cstring>
#include "DirectoryItem.h"

void DirectoryItem::Save(const string& fileSystemName, int32_t offset) {

    fstream stream(fileSystemName, std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(offset, ios_base::beg);

    stream.write((char*)&inode, sizeof(inode));
    stream.write((char*)&item_name, sizeof(item_name));

    stream.close();
}

DirectoryItem::DirectoryItem(const string& fileSystemName, int32_t offset){

    ifstream stream(fileSystemName, std::ios::binary);
    stream.seekg(offset, ios_base::beg);

    stream.read((char*)&inode, sizeof(inode));
    stream.read((char*)&item_name, sizeof(item_name));

    stream.close();
}


DirectoryItem::DirectoryItem(int32_t inode, char item_name[12]){
    this->inode = inode;
    strcpy(this->item_name, item_name);
}
