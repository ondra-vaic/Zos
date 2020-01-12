//
// Created by me on 1/10/20.
//

#ifndef ZOS_DIRECTORYITEM_H
#define ZOS_DIRECTORYITEM_H

#include <stdint-gcc.h>
#include <fstream>
using namespace std;

struct DirectoryItem {
    int32_t inode;                   //inode odpovídající souboru
    char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak

    DirectoryItem() = default;
    DirectoryItem(const string& fileSystemName, int32_t offset);
    DirectoryItem(int32_t inode, char item_name[12]);
    void Save(const string& fileSystemName, int32_t offset);
};


#endif //ZOS_DIRECTORYITEM_H
