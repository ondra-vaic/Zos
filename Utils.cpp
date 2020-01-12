//
// Created by me on 1/10/20.
//

#include "Utils.h"

#include <iostream>
#include <sstream>

vector<string> Utils::GetCommand(){
    vector<string> commandSplitStrings;
    string command;
    string tmp;
    getline(cin, command);
    stringstream X(command);

    while (getline(X, tmp, ' ')) {
        commandSplitStrings.push_back(tmp);
    }

    return commandSplitStrings;
}


bool Utils::GetBit(unsigned char num, unsigned char pos){
    return num & (1 << pos);
}

unsigned char Utils::SetBit(unsigned char num, unsigned char pos){
    return num |= 1 << pos;
}

char* Utils::CreateIdentifier(string s){
    char* identifier = new char[12];
    for (int i = 0; i < s.size(); ++i) {
        identifier[i] = s[i];
    }

    for (int i = s.size(); i < 12; ++i) {
        identifier[i] = '\0';
    }
    return identifier;
}

void Utils::SetZeros(const string& fileSystemName, int32_t offset, int32_t size){
    fstream stream(fileSystemName, std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(offset, ios_base::beg);

    for (int j = 0; j < size; ++j) {
        unsigned char zero = 0;
        stream.write((char*)&zero, sizeof(zero));
    }

    stream.close();
}
