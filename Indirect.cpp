//
// Created by me on 1/12/20.
//

#include "Indirect.h"
#include "PseudoInode.h"

void Indirect::Save(const string& fileSystemName, int32_t offset) {
    fstream stream(fileSystemName, std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(offset, ios_base::beg);

    for (int& i : direct) {
        stream.write((char*)&i, sizeof(i));
    }

    stream.close();
}

Indirect::Indirect(const string& fileSystemName, int32_t offset){
    ifstream stream(fileSystemName, std::ios::binary);
    stream.seekg(offset, ios_base::beg);

    for (int& i : direct) {
        stream.read((char*)&i, sizeof(i));
    }

    stream.close();
}

Indirect::Indirect(){
    for (int& i : direct) {
        i = FREE_DATA_PART;
    }
}