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
#include "PseudoInode.h"

using namespace std;

class FileSystem {

private:
    bool format(const vector<string>& params);
    bool list(const vector<string>& params);
    bool makeDirectory(const vector<string>& params);
    bool changeDirectory(const vector<string>& params);
    bool inCopy(const vector<string>& params);
    bool outCopy(const vector<string>& params);
    bool load(const vector<string>& params);
    bool pwd(const vector<string>& params);
    bool info(const vector<string>& params);
    bool cat(const vector<string>& params);

    vector<int32_t> getReferencedClusters(int32_t inode);
    bool setFirstEmptyReferenceTo(int32_t parent, int32_t address, int32_t size);
    string getFileContents(int32_t fileInodeAddress);

    int32_t allocateIndirect();
    int32_t allocateCluster();
    int32_t allocateInode();
    void freeInode(int32_t address);
    void freeCluster(int32_t address);

    void bindCommands();
    void loadStructures();

    bool getParentDirectoryAtPath(string& path, DirectoryItem& file);
    bool getFileInDirectory(DirectoryItem directory, const string& fileName, DirectoryItem& file);
    string getCurrentPathDescriptor();
    PseudoInode getCurrentDirInode();
    int32_t createNewFile(int32_t parentDirectoryInodeAddress, const string& fileName);
    bool fileExist(const string& fileName, int32_t inodeAddress);
    bool isDirectory(int32_t inodeAddress);

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
