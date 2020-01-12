//
// Created by me on 1/10/20.
//

#ifndef ZOS_UTILS_H
#define ZOS_UTILS_H

#include <vector>
#include <string>
using namespace std;

class Utils {
public:
    static vector<string> GetCommand();

    static bool GetBit(unsigned char num, unsigned char pos);
    static unsigned char SetBit(unsigned char num, unsigned char pos);
    static char* createIdentifier(string s);
};


#endif //ZOS_UTILS_H

/*
bool FileSystem::makeDirectory(const vector<string>& params){

    //getting empty index
    int32_t emptyClusterIndex = clusterBitmap.GetEmptyIndex();
    int32_t emptyInodeIndex = inodeBitmap.GetEmptyIndex();

    //calculating addresses
    int32_t emptyInode = superBlock.inode_start_address + emptyInodeIndex * sizeof(PseudoInode);
    int32_t emptySpace = superBlock.data_start_address + emptyClusterIndex * CLUSTER_SIZE_B;
    setFirstFreeInodeTo(currentDir.inode, emptySpace);

    //name in correct format
    char* directoryName = Utils::createIdentifier(params[1]);

    DirectoryItem directory(emptyInode, directoryName);
    directory.Save(name, emptySpace);

    delete [] directoryName;

    //mark as set
    clusterBitmap.Set(emptyClusterIndex, true, name, superBlock.cluster_bitmap_start_address);
    clusterBitmap.Set(emptyInodeIndex, true, name, superBlock.inode_bitmap_start_address);
    return true;
}
*/