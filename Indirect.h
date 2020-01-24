//
// Created by me on 1/12/20.
//

#ifndef ZOS_INDIRECT_H
#define ZOS_INDIRECT_H

#include <stdint-gcc.h>
#include <fstream>
#include "FileSystem.h"
using namespace std;

struct Indirect{
    int32_t direct[NUM_DIRECT_IN_CLUSTER];

    Indirect();
    Indirect(const string& fileSystemName, int32_t offset);
    void Save(const string& fileSystemName, int32_t offset);
};


#endif //ZOS_INDIRECT_H
