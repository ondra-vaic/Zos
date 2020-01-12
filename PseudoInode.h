//
// Created by me on 1/10/20.
//

#ifndef ZOS_PSEUDOINODE_H
#define ZOS_PSEUDOINODE_H

#include <stdint-gcc.h>
#include <fstream>
using namespace std;

const int32_t ID_ITEM_FREE = 0;
const int32_t FREE_DATA_PART = 0;
const int32_t NUM_DIRECT_NODES = 5;

struct PseudoInode {
    int32_t nodeid;                 //ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
    int32_t dot;
    int32_t dotDot;

    bool isDirectory;               //soubor, nebo adresar
    int8_t references;              //počet odkazů na i-uzel, používá se pro hardlinky
    int32_t file_size;              //velikost souboru v bytech
    int32_t direct[5];              // přímý odkaz na datové bloky
    int32_t indirect1;              // 1. nepřímý odkaz (odkaz - datové bloky)
    int32_t indirect2;              // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)

    PseudoInode();
    PseudoInode(const string& fileSystemName, int32_t offset);
    void Save(const string& fileSystemName, int32_t offset);

};


#endif //ZOS_PSEUDOINODE_H
