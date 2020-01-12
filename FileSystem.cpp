//
// Created by me on 1/10/20.
//

#include <iostream>
#include <vector>
#include <functional>
#include <cstring>
#include "FileSystem.h"
#include "Utils.h"
#include "SuperBlock.h"
#include "PseudoInode.h"
#include "DirectoryItem.h"
#include "Bitmap.h"

using namespace std;
using namespace std::placeholders;

const int32_t CLUSTER_SIZE_B = 512;
const int32_t FILE_EXPECTED_MIN_SIZE = 2 * 1024;

FileSystem::FileSystem(char* name){
    this->name = string(name);
    bindCommands();
    loadStructures();
}

bool FileSystem::format(const vector<string>& params){

    int32_t diskSize = stoi(params[1]);
    int32_t sizeInBytes = diskSize * 1024 * 1024;
    int32_t clusterCount = sizeInBytes / CLUSTER_SIZE_B;
    int32_t inodeCount = sizeInBytes / FILE_EXPECTED_MIN_SIZE;

    int32_t clusterBitmapStartAddress = sizeof(SuperBlock);

    //address is in bytes, so clusterCount / 8, +1 to align
    int32_t inodeBitmapStartAddress = clusterBitmapStartAddress + clusterCount / 8 + 1;
    int32_t inodeStartAddress = inodeBitmapStartAddress + inodeCount / 8 + 1;

    int32_t dataStartAddress = inodeStartAddress + inodeCount * sizeof(PseudoInode);

    //super block
    superBlock = SuperBlock{"test", "prvni fs", diskSize,
                            CLUSTER_SIZE_B, clusterCount, inodeCount,
                            clusterBitmapStartAddress, inodeBitmapStartAddress
                            , inodeStartAddress, dataStartAddress};

    superBlock.Save(name, 0);

    //cluster bit map
    clusterBitmap = Bitmap(clusterCount);
    clusterBitmap.Set(0, true, name, clusterBitmapStartAddress);

    //inode bit map
    inodeBitmap = Bitmap(inodeCount);
    inodeBitmap.Set(0, true, name, inodeBitmapStartAddress);

    //create root inode
    PseudoInode rootInode;
    rootInode.isDirectory = true;
    rootInode.Save(name, inodeStartAddress);

    //create the rest of inodes
    for (int32_t i = 1; i < inodeCount; ++i) {
        PseudoInode inode;
        inode.Save(name, inodeStartAddress + i * sizeof(PseudoInode));
    }

    //create root directory, it is saved at the start of data part and points to first inode
    root = DirectoryItem{inodeStartAddress, "root"};
    root.Save(name, dataStartAddress);

    currentDir = root;

//    DirectoryItem a{inodeStartAddress, "a"};
//    a.Save(name, dataStartAddress + sizeof(DirectoryItem));
//
//    DirectoryItem b{inodeStartAddress, "b"};
//    b.Save(name, dataStartAddress + sizeof(DirectoryItem) * 2);
//
//    DirectoryItem c{inodeStartAddress, "c"};
//    c.Save(name, dataStartAddress + sizeof(DirectoryItem) * 3);
//
//    DirectoryItem d{inodeStartAddress, "d"};
//    d.Save(name, dataStartAddress + sizeof(DirectoryItem) * 4);
//
//    DirectoryItem e{inodeStartAddress, "e"};
//    e.Save(name, dataStartAddress + sizeof(DirectoryItem) * 5);
//
//
//    rootInode.direct[0] = dataStartAddress + sizeof(DirectoryItem);
//    rootInode.direct[1] = dataStartAddress + sizeof(DirectoryItem) * 2;
//    rootInode.direct[2] = dataStartAddress + sizeof(DirectoryItem) * 3;
//    rootInode.direct[3] = dataStartAddress + sizeof(DirectoryItem) * 4;
//    rootInode.direct[4] = dataStartAddress + sizeof(DirectoryItem) * 5;
//
//    PseudoInode indirect{name, inodeStartAddress + (int32_t)sizeof(indirect)};
//    rootInode.indirect1 = inodeStartAddress + sizeof(indirect);
//
//    //indirect.direct[4] = dataStartAddress + sizeof(DirectoryItem);
//    indirect.direct[3] = dataStartAddress + sizeof(DirectoryItem) * 2;
//    indirect.direct[2] = dataStartAddress + sizeof(DirectoryItem) * 3;
//    indirect.direct[1] = dataStartAddress + sizeof(DirectoryItem) * 4;
//    indirect.direct[0] = dataStartAddress + sizeof(DirectoryItem) * 5;
//
//    indirect.Save(name,  rootInode.indirect1);
//    rootInode.Save(name, inodeStartAddress);

    return true;
}

bool FileSystem::list(const vector<string>& params){
    vector<int32_t> inodes = getReferencedInodes(currentDir.inode);
    for (int inode : inodes) {
        DirectoryItem file(name, inode);
        cout << file.item_name << endl;
    }
    return true;
}

vector<int32_t> FileSystem::getReferencedInodes(int32_t inode){
    vector<int32_t> nodes;
    PseudoInode mainInode(name, inode);

    //add all direct nodes
    for(int32_t direct : mainInode.direct){
        if(direct != ID_ITEM_FREE){
            nodes.push_back(direct);
        }
        else{
            return nodes;
        }
    }

    if(mainInode.indirect1 == FREE_DATA_PART)
        return nodes;


    PseudoInode indirectNode(name, mainInode.indirect1);
    //add all direct nodes of the first indirect
    for(int32_t direct : indirectNode.direct){
        if(direct != ID_ITEM_FREE){
            nodes.push_back(direct);
        }
        else{
            return nodes;
        }
    }

    if(mainInode.indirect2 == FREE_DATA_PART)
        return nodes;

    //for all direct nodes of the second indirect
    PseudoInode doubleIndirectNode(name, mainInode.indirect2);
    for(int32_t indirect : doubleIndirectNode.direct) {

        PseudoInode simpleIndirect{name, indirect};
        //add all direct nodes of the simple indirect
        for(int32_t direct : simpleIndirect.direct){
            if(direct != ID_ITEM_FREE){
                nodes.push_back(direct);
            }
            else{
                return nodes;
            }
        }
    }

    return nodes;
}

//can be optimized by traversing backwards
bool FileSystem::setFirstFreeInodeTo(int32_t parent, int32_t address){
    vector<int32_t> clusters;
    PseudoInode mainInode(name, parent);

    //add all direct nodes
    for(int32_t& direct : mainInode.direct){;
        if(direct == ID_ITEM_FREE){
            direct = address;
            mainInode.Save(name, parent);
            return true;
        }
    }

    if(mainInode.indirect1 == ID_ITEM_FREE){

    }

    PseudoInode indirectNode(name, mainInode.indirect1);
    //add all direct nodes of the first indirect
    for(int32_t& direct : indirectNode.direct){
        if(direct == ID_ITEM_FREE){
            direct = address;
            mainInode.Save(name, parent);
            return true;
        }
    }

    //for all direct nodes of the second indirect
    PseudoInode doubleIndirectNode(name, mainInode.indirect2);
    for(int32_t indirect : doubleIndirectNode.direct) {

        PseudoInode simpleIndirect{name, indirect};
        for(int32_t& direct : simpleIndirect.direct){
            if(direct == ID_ITEM_FREE){
                direct = address;
                mainInode.Save(name, parent);
                return true;
            }
        }
    }

    return false;
}


int32_t FileSystem::allocateInode(){
    int32_t emptyInodeIndex = inodeBitmap.GetEmptyIndex();
    int32_t emptyInode = superBlock.inode_start_address + emptyInodeIndex * sizeof(PseudoInode);
    inodeBitmap.Set(emptyInodeIndex, true, name, superBlock.inode_bitmap_start_address);
    return emptyInode;
}

int32_t FileSystem::allocateCluster(){
    int32_t emptyClusterIndex = clusterBitmap.GetEmptyIndex();
    int32_t emptySpace = superBlock.data_start_address + emptyClusterIndex * CLUSTER_SIZE_B;
    clusterBitmap.Set(emptyClusterIndex, true, name, superBlock.cluster_bitmap_start_address);
    return emptySpace;
}

bool FileSystem::makeDirectory(const vector<string>& params){

    int32_t emptySpace = allocateCluster();
    int32_t emptyInode = allocateInode();

    setFirstFreeInodeTo(currentDir.inode, emptySpace);

    //name in correct format
    char* directoryName = Utils::createIdentifier(params[1]);

    DirectoryItem directory(emptyInode, directoryName);
    directory.Save(name, emptySpace);

    delete [] directoryName;

    return true;
}

void FileSystem::bindCommands(){
    commandMap["f"] = bind(&FileSystem::format, this, _1);
    commandMap["ls"] = bind(&FileSystem::list, this, _1);
    commandMap["m"] = bind(&FileSystem::makeDirectory, this, _1);
}

void FileSystem::loadStructures(){
    ifstream fileSystem(name);
    if(!fileSystem.good()){
        cout << "Create new filesystem with the call <format [num megabytes]>" << endl;
        return;
    }

    superBlock = SuperBlock(name, 0);
    root = DirectoryItem(name, superBlock.data_start_address);
    clusterBitmap = Bitmap(name, superBlock.cluster_bitmap_start_address, superBlock.cluster_count);
    inodeBitmap = Bitmap(name, superBlock.inode_bitmap_start_address, superBlock.inode_count);
    currentDir = root;
}


void FileSystem::Run(){

    while(true){
        cout << "Enter command: " ;

        vector<string> commandSplitStrings = Utils::GetCommand();
        bool result = commandMap[commandSplitStrings[0]](commandSplitStrings);

        if(!result)
            break;
    }
}
