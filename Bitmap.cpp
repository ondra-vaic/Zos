//
// Created by me on 1/11/20.
//

#include <fstream>
#include <iostream>
#include "Bitmap.h"
#include "Utils.h"

Bitmap::Bitmap(int length){
    for (int i = 0; i < length; ++i) {
        bitmap.push_back(false);
    }
}

Bitmap::Bitmap(const string& fileSystemName, int32_t offset, int32_t length){

    ifstream stream(fileSystemName, std::ios::binary);
    stream.seekg(offset, ios_base::beg);

    for (int i = 0; i < length; ++i) {
        unsigned char byte = 0;
        stream.read((char*)&byte, sizeof(byte));

        for (int j = 0; j < 8; ++j) {
            bitmap.push_back(Utils::GetBit(byte, 7 - j));
        }
    }
    
    stream.close();
}

void Bitmap::Save(const string& fileSystemName, int32_t offset){
    fstream stream(fileSystemName, std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(offset, ios_base::beg);

    unsigned char byte = 0;

    int off = offset;
    for (int i = 0; i < bitmap.size(); ++i) {

        if(bitmap[i]){
            byte = Utils::SetBit(byte, (7 - (i % 8)));
        }

        if((i + 1) % 8 == 0){
            stream.write((char*)&byte, sizeof(byte));
            off++;
            byte = 0;
        }
    }

    stream.write((char*)&byte, sizeof(byte));
    stream.close();
}

bool Bitmap::IsSet(int32_t index){
    return bitmap[index];
}

void Bitmap::Set(int32_t index, bool allocated, const string& fileSystemName, int32_t offset){
    bitmap[index] = allocated;
    Save(fileSystemName, offset);
}

int32_t Bitmap::GetEmptyIndex(){

    for (int i = 0; i < bitmap.size(); ++i) {
        if(bitmap[i] == false){
            return i;
        }
    }

    return -1;
}