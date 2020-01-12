//
// Created by me on 1/10/20.
//


#include <iostream>
#include "PseudoInode.h"

PseudoInode::PseudoInode(){
    nodeid = ID_ITEM_FREE;
    isDirectory = false;
    references = 0;
    file_size = 0;
    dot = FREE_DATA_PART;
    dotDot = FREE_DATA_PART;

    for (int& i : direct) {
        i = FREE_DATA_PART;
    }

    indirect1 = FREE_DATA_PART;
    indirect2 = FREE_DATA_PART;
}

PseudoInode::PseudoInode(const string& fileSystemName, int offset){

    ifstream stream(fileSystemName, std::ios::binary);
    stream.seekg(offset, ios_base::beg);

    stream.read((char*)&nodeid, sizeof(nodeid));
    stream.read((char*)&isDirectory, sizeof(isDirectory));
    stream.read((char*)&references, sizeof(references));
    stream.read((char*)&file_size, sizeof(file_size));

    for (int& i : direct) {
        stream.read((char*)&i, sizeof(i));
    }

    stream.read((char*)&dot, sizeof(dot));
    stream.read((char*)&dotDot, sizeof(dotDot));
    stream.read((char*)&indirect1, sizeof(indirect1));
    stream.read((char*)&indirect2, sizeof(indirect2));

    stream.close();
}

void PseudoInode::Save(const string& fileSystemName, int offset){

    fstream stream(fileSystemName, std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(offset, ios_base::beg);

    stream.write((char*)&nodeid, sizeof(nodeid));
    stream.write((char*)&isDirectory, sizeof(isDirectory));
    stream.write((char*)&references, sizeof(references));
    stream.write((char*)&file_size, sizeof(file_size));

    for (int& i : direct) {
        stream.write((char*)&i, sizeof(i));
    }

    stream.write((char*)&dot, sizeof(dot));
    stream.write((char*)&dotDot, sizeof(dotDot));
    stream.write((char*)&indirect1, sizeof(indirect1));
    stream.write((char*)&indirect2, sizeof(indirect2));

    stream.close();
}

