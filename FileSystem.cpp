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
#include "Indirect.h"
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace std;
using namespace std::placeholders;


FileSystem::FileSystem(char* name){
    this->name = string(name);
    bindCommands();
    loadStructures();
}

vector<int32_t> FileSystem::getReferencedClusters(int32_t inode){
    vector<int32_t> nodes;
    PseudoInode mainInode(name, inode);

    //add all direct nodes
    for(int32_t direct : mainInode.direct){
        if(direct != ID_ITEM_FREE){
            nodes.push_back(direct);
        }
    }

    if(mainInode.indirect1 == ID_ITEM_FREE)
        return nodes;

    Indirect indirectNode(name, mainInode.indirect1);
    //add all direct nodes of the first indirect
    for(int32_t direct : indirectNode.direct){
        if(direct != ID_ITEM_FREE){
            nodes.push_back(direct);
        }
    }

    if(mainInode.indirect2 == ID_ITEM_FREE)
        return nodes;

    //for all direct nodes of the second indirect
    Indirect doubleIndirectNode(name, mainInode.indirect2);
    for(int32_t indirect : doubleIndirectNode.direct) {

        if(indirect == ID_ITEM_FREE)
            return nodes;

        Indirect simpleIndirect{name, indirect};
        //add all direct nodes of the simple indirect
        for(int32_t direct : simpleIndirect.direct){
            if(direct != ID_ITEM_FREE){
                nodes.push_back(direct);
            }
        }
    }

    return nodes;
}

bool FileSystem::setFirstEmptyReferenceTo(int32_t parent, int32_t address, int32_t size){

    PseudoInode inode(name, parent);
    inode.file_size += size;

    for(int32_t& direct : inode.direct){
        if(direct == ID_ITEM_FREE){
            direct = address;
            inode.Save(name, parent);
            return true;
        }
    }

    if(inode.indirect1 == ID_ITEM_FREE){
        inode.indirect1 = allocateIndirect();
    }

    Indirect indirect(name, inode.indirect1);
    for(int32_t& directAddress : indirect.direct){
        if(directAddress == ID_ITEM_FREE){
            directAddress = address;
            inode.Save(name, parent);
            indirect.Save(name, inode.indirect1);
            return true;
        }
    }

    if(inode.indirect2 == ID_ITEM_FREE){
        inode.indirect2 = allocateIndirect();
    }

    //for all direct nodes of the second indirect
    Indirect indirect2(name, inode.indirect2);
    for(int32_t& simpleIndirectAddress : indirect2.direct) {

        if(simpleIndirectAddress == ID_ITEM_FREE){
            simpleIndirectAddress = allocateIndirect();
        }

        Indirect simpleIndirect{name, simpleIndirectAddress};
        for(int32_t& directAddress : simpleIndirect.direct){
            if(directAddress == ID_ITEM_FREE){
                directAddress = address;
                simpleIndirect.Save(name, simpleIndirectAddress);
                indirect2.Save(name, inode.indirect2);
                inode.Save(name, parent);
                return true;
            }
        }
    }

    return false;
}

void FileSystem::removeDirectoryItemReferenceAt(int32_t parentInodeAddress, int32_t address){
    vector<int32_t> parentReferencedClusters = getReferencedClusters(parentInodeAddress);
    cleanInode(parentInodeAddress);

    for (int32_t cluster : parentReferencedClusters) {
        if(cluster != address){
            setFirstEmptyReferenceTo(parentInodeAddress, cluster, 0);
        }
    }
}

void FileSystem::cleanInode(int32_t inodeAddress){

    PseudoInode inode(name, inodeAddress);

    //add all direct nodes
    for(int32_t& direct : inode.direct){
        if(direct != ID_ITEM_FREE){
            direct = ID_ITEM_FREE;
        }
    }

    if(inode.indirect1 != ID_ITEM_FREE){
        freeCluster(inode.indirect1);
        inode.indirect1 = ID_ITEM_FREE;
    }

    if(inode.indirect2 != ID_ITEM_FREE){

        Indirect doubleIndirectNode(name, inode.indirect2);
        for (int32_t& indirect : doubleIndirectNode.direct) {
            if(indirect != ID_ITEM_FREE){
                freeCluster(indirect);
            }
        }

        freeCluster(inode.indirect2);
        inode.indirect2 = ID_ITEM_FREE;
    }

    inode.Save(name, inodeAddress);
}

bool FileSystem::format(const vector<string>& params){

    remove(name.c_str());
    ofstream outfile (name);
    outfile.close();

    int32_t diskSize = stoi(params[1]);
    int32_t sizeInBytes = diskSize * 1024 * 1024;

    int32_t inodeCount = sizeInBytes / FILE_EXPECTED_MIN_SIZE;

    //without space for cluster bitmap
    int32_t metaDataPartSize = sizeof(SuperBlock) + inodeCount * sizeof(PseudoInode) + inodeCount / 8 + 1;

    //figure out number of clusters, +1 because one bit extra for every cluster is for bitmap
    int32_t clusterCount = ((sizeInBytes - metaDataPartSize) * 8) / (CLUSTER_SIZE_B * 8 + 1);

    //int32_t clusterCount = sizeInBytes / CLUSTER_SIZE_B;
    int32_t clusterBitmapStartAddress = sizeof(SuperBlock);

    //address is in bytes, so clusterCount / 8, +1 because integer division
    int32_t inodeBitmapStartAddress = clusterBitmapStartAddress + clusterCount / 8 + 1;
    int32_t inodeStartAddress = inodeBitmapStartAddress + inodeCount / 8 + 1;

    int32_t dataStartAddress = inodeStartAddress + inodeCount * sizeof(PseudoInode);

    cout << clusterCount  <<endl;
    //super block
    superBlock = SuperBlock(Utils::CreateIdentifier("test"), Utils::CreateIdentifier("prvni fs"), diskSize,
                            CLUSTER_SIZE_B, clusterCount, inodeCount,
                            clusterBitmapStartAddress, inodeBitmapStartAddress
            , inodeStartAddress, dataStartAddress);

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
    rootInode.dot = dataStartAddress;
    rootInode.dotDot = dataStartAddress;
    rootInode.Save(name, inodeStartAddress);

    //create the rest of inodes
    for (int32_t i = 1; i < inodeCount; ++i) {
        PseudoInode inode;
        inode.Save(name, inodeStartAddress + i * sizeof(PseudoInode));
    }

    //create root directory, it is saved at the start of data part and points to first inode
    root = DirectoryItem(inodeStartAddress, Utils::CreateIdentifier("/"));
    root.Save(name, dataStartAddress);

    currentDir = root;
    return true;
}

bool FileSystem::list(const vector<string>& params){

    DirectoryItem directory;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), directory))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(directory, params[1], directory)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!isDirectory(directory.inode)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    vector<int32_t> clusters = getReferencedClusters(directory.inode);
    for (int32_t cluster : clusters) {
        DirectoryItem file(name, cluster);
        PseudoInode fileInode(name, file.inode);
        if(fileInode.isDirectory){
            cout << "+";
        }else{
            cout << "-";
        }
        cout << file.item_name << endl;
    }
    return true;
}

bool FileSystem::mkdir(const vector<string>& params){

    DirectoryItem workingDirectory;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), workingDirectory))
    {
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(fileExist(params[1], workingDirectory.inode))
    {
        cout << "EXIST" <<endl;
        return false;
    }

    if(params[1].size() > 11)
    {
        cout << "File name is too long" <<endl;
        return false;
    }

    if(!canCreateFile(CLUSTER_SIZE_B)){
        cout << "Not enough space" << endl;
        return false;
    }

    int32_t newFileAddress = createNewFile(workingDirectory.inode, params[1]);

    DirectoryItem directory(name, newFileAddress);

    PseudoInode directoryInode(name, directory.inode);
    directoryInode.isDirectory = true;
    directoryInode.Save(name, directory.inode);

    return true;
}

bool FileSystem::cd(const vector<string>& params){

    DirectoryItem workingDirectory;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), workingDirectory))
    {
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(workingDirectory, params[1], workingDirectory)){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(!isDirectory(workingDirectory.inode)){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    currentDir = workingDirectory;
    return true;
}

bool FileSystem::incp(const vector<string>& params){

    DirectoryItem destination;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[2]), destination))
    {
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(!isDirectory(destination.inode)){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(fileExist(params[2], destination.inode))
    {
        cout << "EXIST" <<endl;
        return false;
    }

    ifstream fileToCopy (params[1], ios::in | ios::binary);
    if(!fileToCopy.good()){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    fileToCopy.seekg (0, ifstream::end);
    int32_t fileSize = fileToCopy.tellg();
    fileToCopy.seekg (0, ifstream::beg);

    if(!canCreateFile(fileSize)){
        cout << "Not enough space" << endl;
        return false;
    }

    int32_t newFileAddress = createNewFile(destination.inode, params[2]);
    if(newFileAddress == -1){
        return false;
    }

    DirectoryItem newFile(name, newFileAddress);
    fstream fileSystem(name, std::ios::binary | std::ios::in | std::ios::out);

    int numClusters = 1 + fileSize / CLUSTER_SIZE_B;

    for (int i = 0; i < numClusters; ++i) {

        int32_t bufferLength = CLUSTER_SIZE_B;
        if(i == numClusters - 1){
            bufferLength = fileSize % CLUSTER_SIZE_B;
        }
        char buffer[bufferLength];

        fileToCopy.read(buffer, bufferLength);

        int32_t clusterAddress = allocateCluster();
        fileSystem.seekp(clusterAddress, ios_base::beg);
        fileSystem.write((char*)&buffer, sizeof(buffer));

        setFirstEmptyReferenceTo(newFile.inode, clusterAddress, bufferLength);
    }

    fileSystem.close();
    fileToCopy.close();

    return true;
}

bool FileSystem::outcp(const vector<string>& params){

    DirectoryItem file;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), file))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(file, params[1], file)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(isDirectory(file.inode)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    fstream outStream(params[2], std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    if(!outStream.good()){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    string fileContents = getFileContents(file.inode);
    outStream.write(fileContents.c_str(), sizeof(char) * fileContents.size());
    outStream.close();
    return true;
}

string FileSystem::getFileContents(int32_t fileInodeAddress){

    string contents;

    PseudoInode fileInode(name, fileInodeAddress);
    int32_t fileSize = fileInode.file_size;
    vector<int32_t> referencedClusters = getReferencedClusters(fileInodeAddress);

    ifstream fileSystem (name, ios::in | ios::binary);

    for (int i = 0; i < referencedClusters.size(); ++i) {

        int32_t bufferLength = CLUSTER_SIZE_B;
        if(i == referencedClusters.size() - 1){
            bufferLength = fileSize % CLUSTER_SIZE_B;
        }
        char buffer[bufferLength];
        fileSystem.seekg(referencedClusters[i], ios_base::beg);
        fileSystem.read(buffer, bufferLength);

        contents.insert(contents.size(), buffer, bufferLength);
    }

    fileSystem.close();
    return contents;
}

bool FileSystem::load(const vector<string>& params){

    ifstream commands(params[1]);

    if(!commands.good()){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    string line;
    while (getline(commands, line)){
        vector<string> commandSplitStrings;
        string tmp;

        stringstream X(line);

        cout << getCurrentPathDescriptor() << ":" << line <<endl;


        while (getline(X, tmp, ' ')) {
            commandSplitStrings.push_back(tmp);
        }
        commandMap[commandSplitStrings[0]](commandSplitStrings);
    }

    commands.close();
    return true;
}

bool FileSystem::pwd(const vector<string>& params){
    cout << getCurrentPathDescriptor() << endl;
    return true;
}

bool FileSystem::info(const vector<string>& params){
    DirectoryItem file;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), file))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(file, params[1], file)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    PseudoInode fileInode(name, file.inode);

    int inodeIndex = (file.inode - superBlock.inode_start_address) / sizeof(PseudoInode);
    cout << file.item_name << " - " << fileInode.file_size << " - i-node " << inodeIndex << endl;
    return true;
}

bool FileSystem::cat(const vector<string>& params){
    DirectoryItem file;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), file))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(file, params[1], file)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(isDirectory(file.inode)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    string fileContents = getFileContents(file.inode);

    cout << fileContents << endl;

    return true;
}

bool FileSystem::rmdir(const vector<string>& params){

    DirectoryItem directory;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), directory))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(directory, params[1], directory)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!isDirectory(directory.inode)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    vector<int32_t> inodes = getReferencedClusters(directory.inode);
    if(!inodes.empty()){
        cout << "NOT EMPTY" <<endl;
        return false;
    }

    PseudoInode directoryInode(name, directory.inode);
    int32_t parentInodeAddress = DirectoryItem(name, directoryInode.dotDot).inode;
    removeDirectoryItemReferenceAt(parentInodeAddress, directoryInode.dot);

    freeInode(directory.inode);
    freeCluster(directoryInode.dot);

    return true;
}

bool FileSystem::rm(const vector<string>& params){
    DirectoryItem file;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), file))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(file, params[1], file)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(isDirectory(file.inode)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    vector<int32_t> filesClusters = getReferencedClusters(file.inode);
    for (int filesCluster : filesClusters) {
        freeCluster(filesCluster);
    }

    PseudoInode fileInode(name, file.inode);
    int32_t parentInodeAddress = DirectoryItem(name, fileInode.dotDot).inode;

    removeDirectoryItemReferenceAt(parentInodeAddress, fileInode.dot);

    freeInode(file.inode);
    freeCluster(fileInode.dot);

    return true;
}

bool FileSystem::mv(const vector<string>& params){

    DirectoryItem destination;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[2]), destination))
    {
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(destination, params[2], destination)){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(!isDirectory(destination.inode)){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(fileExist(params[1], destination.inode))
    {
        cout << "EXIST" <<endl;
        return false;
    }

    DirectoryItem sourceParentDir;
    DirectoryItem source;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), sourceParentDir))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(sourceParentDir, params[1], source)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    PseudoInode sourceInode(name, source.inode);

    removeDirectoryItemReferenceAt(sourceParentDir.inode, sourceInode.dot);
    setFirstEmptyReferenceTo(destination.inode, sourceInode.dot, sourceInode.file_size);

    sourceInode.dotDot = PseudoInode(name, destination.inode).dot;
    sourceInode.Save(name, source.inode);

    return true;
}

bool FileSystem::cp(const vector<string>& params){

    DirectoryItem destination;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[2]), destination))
    {
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(!isDirectory(destination.inode)){
        cout << "PATH NOT FOUND" <<endl;
        return false;
    }

    if(fileExist(params[2], destination.inode))
    {
        cout << "EXIST" <<endl;
        return false;
    }

    DirectoryItem sourceParentDir;
    DirectoryItem source;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), sourceParentDir))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(sourceParentDir, params[1], source)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    PseudoInode sourceInode(name, source.inode);

    if(!canCreateFile(sourceInode.file_size)){
        cout << "Not enough space" << endl;
        return false;
    }

    vector<int32_t> referencedClusters = getReferencedClusters(source.inode);

    DirectoryItem newFile = DirectoryItem(name, createNewFile(destination.inode, params[2]));

    fstream fileSystem(name, std::ios::binary | std::ios::in | std::ios::out);
    int numClusters = 1 + sourceInode.file_size / CLUSTER_SIZE_B;

    for (int i = 0; i < numClusters; ++i) {

        int32_t bufferLength = CLUSTER_SIZE_B;
        if(i == numClusters - 1){
            bufferLength = sourceInode.file_size % CLUSTER_SIZE_B;
        }
        char buffer[bufferLength];

        fileSystem.seekp(referencedClusters[i], ios_base::beg);
        fileSystem.read(buffer, bufferLength);

        int32_t clusterAddress = allocateCluster();
        fileSystem.seekp(clusterAddress, ios_base::beg);
        fileSystem.write((char*)&buffer, sizeof(buffer));

        setFirstEmptyReferenceTo(newFile.inode, clusterAddress, bufferLength);
    }

    fileSystem.close();

    return true;
}

bool FileSystem::check(const vector<string>& params){

    vector<int32_t> inodeAddresses = getAllInodeAddresses();
    for (int32_t inodeAddress : inodeAddresses) {
        if(inodeAddress == root.inode)
            continue;

        PseudoInode inode(name, inodeAddress);

        if(!inode.isDirectory){

            int32_t numClusters = getReferencedClusters(inodeAddress).size();
            DirectoryItem currentFile(name, inode.dot);

            if(numClusters != inode.file_size / CLUSTER_SIZE_B + 1){
                cout << "FILE " << currentFile.item_name << " IS CORRUPTED" <<endl;
            }

            DirectoryItem parentDirectory(name, inode.dotDot);
            bool parentContainsCurrentFile = false;

            vector<int32_t> fileAddresses = getReferencedClusters(parentDirectory.inode);

            for (int32_t fileAddress : fileAddresses) {
                DirectoryItem file(name, fileAddress);
                string fileName = file.item_name;

                if(fileName == currentFile.item_name){
                    parentContainsCurrentFile = true;
                }
            }

            if(!parentContainsCurrentFile){
                cout << "FILE " << currentFile.item_name << " IS IN NO DIRECTORY" <<endl;
            }
        }
    }
    
    return true;
}

bool FileSystem::corrupt(const vector<string>& params){
    DirectoryItem file;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), file))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(file, params[1], file)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(isDirectory(file.inode)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    PseudoInode inode(name, file.inode);
    inode.file_size = 165;
    inode.Save(name, file.inode);
    return true;
}

bool FileSystem::corrupt2(const vector<string>& params){
    DirectoryItem file;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[1]), file))
    {
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(!getFileInDirectory(file, params[1], file)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    if(isDirectory(file.inode)){
        cout << "FILE NOT FOUND" <<endl;
        return false;
    }

    PseudoInode inode(name, file.inode);
    DirectoryItem parentDirectory(name, inode.dotDot);
    removeDirectoryItemReferenceAt(parentDirectory.inode, inode.dot);

    return true;
}

vector<int32_t> FileSystem::getAllInodeAddresses(){
    vector<int32_t> allocatedInodesIndices = inodeBitmap.GetAllocatedIndices();
    vector<int32_t> inodeAddresses;
    for (int i = 0; i < allocatedInodesIndices.size(); ++i) {
        int32_t address = superBlock.inode_start_address + allocatedInodesIndices[i] * sizeof(PseudoInode);
        inodeAddresses.push_back(address);
    }
    return inodeAddresses;
}

void FileSystem::bindCommands(){

    commandMap["format"] = bind(&FileSystem::format, this, _1);
    commandMap["ls"] = bind(&FileSystem::list, this, _1);
    commandMap["mkdir"] = bind(&FileSystem::mkdir, this, _1);
    commandMap["cd"] = bind(&FileSystem::cd, this, _1);
    commandMap["incp"] = bind(&FileSystem::incp, this, _1);
    commandMap["outcp"] = bind(&FileSystem::outcp, this, _1);
    commandMap["load"] = bind(&FileSystem::load, this, _1);
    commandMap["pwd"] = bind(&FileSystem::pwd, this, _1);
    commandMap["info"] = bind(&FileSystem::info, this, _1);
    commandMap["cat"] = bind(&FileSystem::cat, this, _1);
    commandMap["rmdir"] = bind(&FileSystem::rmdir, this, _1);
    commandMap["rm"] = bind(&FileSystem::rm, this, _1);
    commandMap["mv"] = bind(&FileSystem::mv, this, _1);
    commandMap["cp"] = bind(&FileSystem::cp, this, _1);
    commandMap["check"] = bind(&FileSystem::check, this, _1);
    commandMap["corrupt"] = bind(&FileSystem::corrupt, this, _1);
    commandMap["corrupt2"] = bind(&FileSystem::corrupt2, this, _1);

}

void FileSystem::loadStructures(){
    ifstream fileSystem(name);
    if(!fileSystem.good()){
        cout << "Create new filesystem with the call <format [num megabytes]>" << endl;
        return;
    }
    fileSystem.close();

    superBlock = SuperBlock(name, 0);
    root = DirectoryItem(name, superBlock.data_start_address);
    clusterBitmap = Bitmap(name, superBlock.cluster_bitmap_start_address, superBlock.cluster_count);
    inodeBitmap = Bitmap(name, superBlock.inode_bitmap_start_address, superBlock.inode_count);
    currentDir = root;
}

void FileSystem::Run(){
    while(true){
        cout << getCurrentPathDescriptor() + ":";

        vector<string> commandSplitStrings = Utils::GetCommand();
        if(commandSplitStrings[0] == "x"){
            break;
        }

        if (commandMap.find(commandSplitStrings[0]) == commandMap.end()){
            cout << "Incorrect command" << endl;
            continue;
        }

        bool result = commandMap[commandSplitStrings[0]](commandSplitStrings);
    }
}

int32_t FileSystem::createNewFile(int32_t parentDirectoryInodeAddress, const string& fileName){

    char* directoryName = Utils::CreateIdentifier(fileName);

    PseudoInode parentDirectoryInode(name, parentDirectoryInodeAddress);
    DirectoryItem parentDirectory(name, parentDirectoryInode.dot);

    int32_t emptyInodeAddress = allocateInode();
    int32_t emptySpaceAddress = allocateCluster();

    setFirstEmptyReferenceTo(parentDirectory.inode, emptySpaceAddress, CLUSTER_SIZE_B);

    PseudoInode inode(name, emptyInodeAddress);
    inode.dot = emptySpaceAddress;
    inode.dotDot = parentDirectoryInode.dot;
    inode.Save(name, emptyInodeAddress);

    DirectoryItem directory(emptyInodeAddress, directoryName);
    directory.Save(name, emptySpaceAddress);

    delete [] directoryName;

    return emptySpaceAddress;
}

bool FileSystem::getParentDirectoryAtPath(string& path, DirectoryItem& file){
    size_t pos = 0;
    string token;

    DirectoryItem directoryItem = currentDir;

    if(path[0] == '/'){
        directoryItem = root;
        path.erase(0, 1);
    }

    vector<string> directories;
    while ((pos = path.find('/')) != string::npos) {
        string dir = path.substr(0, pos);
        directories.push_back(dir);
        path.erase(0, pos + 1);
    }

    for(int i = 0; i < directories.size(); ++i){
        if(!getFileInDirectory(directoryItem, directories[i], directoryItem)){
            return false;
        }
        if(!isDirectory(directoryItem.inode)){
            return false;
        }
    }

    file = directoryItem;
    return true;
}

bool FileSystem::getFileInDirectory(DirectoryItem directory, const string& fileName, DirectoryItem& file){

    if(fileName.empty())
        return true; // is root

    if(fileName == ".."){
        PseudoInode inode(name, directory.inode);
        DirectoryItem dotDot(name, inode.dotDot);
        file = dotDot;
        return true;
    }

    if(fileName == "."){
        PseudoInode inode(name, directory.inode);
        DirectoryItem dot(name, inode.dot);
        file = dot;
        return true;
    }

    vector<int32_t> dirs = getReferencedClusters(directory.inode);
    for(int i = 0; i < dirs.size(); ++i) {

        DirectoryItem dir(name, dirs[i]);
        PseudoInode inode(name, dir.inode);

        if(dir.item_name == fileName){
            file = dir;
            return true;
        }
    }
    return false;
}

string FileSystem::getCurrentPathDescriptor(){
    vector<string> dirs;

    PseudoInode inode = getCurrentDirInode();

    while(inode.dotDot != inode.dot){
        DirectoryItem dir(name, inode.dot);
        string dirName = dir.item_name;
        dirs.push_back(dirName);

        DirectoryItem dir2(name, inode.dotDot);
        inode = PseudoInode(name, dir2.inode);
    }

    string path = "/";
    for (int i = 0; i < dirs.size(); ++i) {
        path += dirs[dirs.size() - i - 1];
        if(i < dirs.size() - 1){
            path += "/";
        }
    }

    return path;
}

PseudoInode FileSystem::getCurrentDirInode(){
    return PseudoInode(name, currentDir.inode);
}

bool FileSystem::isDirectory(int32_t inodeAddress){
    return PseudoInode(name, inodeAddress).isDirectory;
}

bool FileSystem::fileExist(const string& fileName, int32_t parentDirInodeAddress){

    vector<int32_t> files = getReferencedClusters(parentDirInodeAddress);

    for(int32_t fileAddress : files) {

        DirectoryItem file(name, fileAddress);

        if(file.item_name == fileName){
            return true;
        }
    }

    return false;
}

int32_t FileSystem::allocateIndirect(){
    Indirect i;
    int32_t address = allocateCluster();
    i.Save(name, address);
    return address;
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

void FileSystem::freeCluster(int32_t address){
    int32_t index = (address - superBlock.data_start_address) / CLUSTER_SIZE_B;
    clusterBitmap.Set(index, false, name, superBlock.cluster_bitmap_start_address);
}

void FileSystem::freeInode(int32_t address){
    PseudoInode empty;
    empty.Save(name, address);
    int32_t index = (address - superBlock.inode_start_address) / sizeof(PseudoInode);
    inodeBitmap.Set(index, false, name, superBlock.inode_bitmap_start_address);
}

bool FileSystem::fileFitsInClusters(int32_t size){
    int32_t numEmptyClusters = clusterBitmap.GetNumFreeSpaces();

    int32_t numDirectClustersNeeded = size / CLUSTER_SIZE_B + 1;
    int32_t numIndirectClustersNeeded = numDirectClustersNeeded / NUM_DIRECT_IN_CLUSTER + 2;
    int32_t totalNumClusters = numDirectClustersNeeded + numIndirectClustersNeeded;

    return numEmptyClusters > totalNumClusters;
}

bool FileSystem::canCreateFile(int32_t size){
    return fileFitsInClusters(size) && inodeBitmap.GetEmptyIndex() != -1;
}