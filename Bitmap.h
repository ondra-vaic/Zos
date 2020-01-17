//
// Created by me on 1/11/20.
//

#ifndef ZOS_BITMAP_H
#define ZOS_BITMAP_H

#include <vector>
#include <string>

using namespace std;

struct Bitmap {

    Bitmap() = default;
    explicit Bitmap(int length);
    Bitmap(const string& fileSystemName, int32_t offset, int32_t length);

    void Save(const string& fileSystemName, int32_t offset);
    bool IsSet(int32_t index);
    void Set(int32_t index, bool allocated, const string& fileSystemName, int32_t offset);
    int32_t GetEmptyIndex();
    vector<int32_t> GetAllocatedIndices();

    vector<bool> bitmap;
};


#endif //ZOS_BITMAP_H
