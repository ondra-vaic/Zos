//
// Created by me on 1/10/20.
//

#ifndef ZOS_FILESYSTEM_H
#define ZOS_FILESYSTEM_H

#include <vector>
#include <string>
#include <functional>
#include <map>
#include "SuperBlock.h"
#include "DirectoryItem.h"
#include "Bitmap.h"

using namespace std;

class FileSystem {

private:
    bool format(const vector<string>& params);
    bool list(const vector<string>& params);
    bool makeDirectory(const vector<string>& params);

    vector<int32_t> getReferencedInodes(int32_t inode);
    bool setFirstFreeInodeTo(int32_t parent, int32_t address);

    int32_t allocateCluster();
    int32_t allocateInode();

    void bindCommands();
    void loadStructures();

    map<string, function<bool(vector<string>)>> commandMap;
    SuperBlock superBlock;
    DirectoryItem root;
    DirectoryItem currentDir;

    Bitmap inodeBitmap;
    Bitmap clusterBitmap;
    string name;

public:
    explicit FileSystem(char* name);
    void Run();
};


#endif //ZOS_FILESYSTEM_H
