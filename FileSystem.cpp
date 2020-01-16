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

const int32_t CLUSTER_SIZE_B = 512 * 20;
const int32_t FILE_EXPECTED_MIN_SIZE = 2 * 1024;

FileSystem::FileSystem(char* name){
    this->name = string(name);
    bindCommands();
    loadStructures();
}

bool FileSystem::format(const vector<string>& params){

    remove(name.c_str());
    ofstream outfile (name);
    outfile.close();

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
    vector<int32_t> inodes = getReferencedClusters(currentDir.inode);
    for (int inode : inodes) {
        DirectoryItem file(name, inode);
        cout << file.item_name << endl;
    }
    return true;
}

vector<int32_t> FileSystem::getReferencedClusters(int32_t inode){
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


    Indirect indirectNode(name, mainInode.indirect1);
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
    Indirect doubleIndirectNode(name, mainInode.indirect2);
    for(int32_t indirect : doubleIndirectNode.direct) {

        Indirect simpleIndirect{name, indirect};
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
bool FileSystem::setFirstEmptyReferenceTo(int32_t parent, int32_t address, int32_t size){

    PseudoInode inode(name, parent);
    inode.file_size += size;

    for(int32_t& direct : inode.direct){;
        if(direct == ID_ITEM_FREE){
            direct = address;
            cout << "saving " << address << " parent " << parent <<endl;
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
            simpleIndirectAddress = allocateInode();
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

bool FileSystem::makeDirectory(const vector<string>& params){

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

    int32_t newFileAddress = createNewFile(workingDirectory.inode, params[1]);
    if(newFileAddress == -1){
        return false;
    }

    DirectoryItem directory(name, newFileAddress);

    PseudoInode directoryInode(name, directory.inode);
    directoryInode.isDirectory = true;
    directoryInode.Save(name, directory.inode);

    return true;
}

bool FileSystem::changeDirectory(const vector<string>& params){

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

bool FileSystem::inCopy(const vector<string>& params){

    DirectoryItem destination;

    if(!getParentDirectoryAtPath(const_cast<string &>(params[2]), destination))
    {
        cout << "PATH NOT FOUND" <<endl;
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

bool FileSystem::outCopy(const vector<string>& params){

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

    fstream outStream(params[2], std::ios::binary | std::ios::in | std::ios::out);
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

    cout << file.item_name << " - " << fileInode.file_size << " - i-node " << file.inode << endl;
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

void FileSystem::bindCommands(){
    commandMap["format"] = bind(&FileSystem::format, this, _1);
    commandMap["ls"] = bind(&FileSystem::list, this, _1);
    commandMap["mkdir"] = bind(&FileSystem::makeDirectory, this, _1);
    commandMap["cd"] = bind(&FileSystem::changeDirectory, this, _1);
    commandMap["incp"] = bind(&FileSystem::inCopy, this, _1);
    commandMap["outcp"] = bind(&FileSystem::outCopy, this, _1);
    commandMap["load"] = bind(&FileSystem::load, this, _1);
    commandMap["pwd"] = bind(&FileSystem::pwd, this, _1);
    commandMap["info"] = bind(&FileSystem::info, this, _1);
    commandMap["cat"] = bind(&FileSystem::cat, this, _1);

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
//
//    DirectoryItem d(name, 14822694);
//    cout << d.item_name <<endl;
//    cout << getFileContents(d.inode) <<endl;

    while(true){
        cout << getCurrentPathDescriptor() + ":";

        vector<string> commandSplitStrings = Utils::GetCommand();
        if(commandSplitStrings[0] == "x"){
            break;
        }

        bool result = commandMap[commandSplitStrings[0]](commandSplitStrings);

        cout << "------" <<endl;
        vector<int32_t> adas = getReferencedClusters(currentDir.inode);
        for (int i = 0; i < adas.size(); ++i) {
            cout << adas[i] <<endl;
        }
        cout << "------" <<endl;
    }
}

int32_t FileSystem::createNewFile(int32_t parentDirectoryInodeAddress, const string& fileName){
    char* directoryName = Utils::CreateIdentifier(fileName);

    PseudoInode parentDirectoryInode(name, parentDirectoryInodeAddress);
    DirectoryItem parentDirectory(name, parentDirectoryInode.dot);

    int32_t emptyInodeAddress = allocateInode();
    int32_t emptySpaceAddress = allocateCluster();

    bool referenceSet = setFirstEmptyReferenceTo(parentDirectory.inode, emptySpaceAddress, CLUSTER_SIZE_B);
    if(!referenceSet){
        cout << "Out of references in inode" << endl;
        freeCluster(emptySpaceAddress);
        freeInode(emptyInodeAddress);
        return -1;
    }

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

bool FileSystem::fileExist(const string& fileName, int32_t inodeAddress){

    vector<int32_t> dirs = getReferencedClusters(inodeAddress);

    for(int i = 0; i < dirs.size(); ++i) {

        DirectoryItem dir(name, dirs[i]);

        if(dir.item_name == fileName){
            return true;
        }
    }

    return false;
}

int32_t FileSystem::allocateIndirect(){
    //save
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
    int32_t index = (address - superBlock.cluster_bitmap_start_address) / CLUSTER_SIZE_B;
    clusterBitmap.Set(index, false, name, superBlock.cluster_bitmap_start_address);
}

void FileSystem::freeInode(int32_t address){
    PseudoInode empty;
    empty.Save(name, address);
    int32_t index = (address - superBlock.inode_bitmap_start_address) / sizeof(PseudoInode);
    inodeBitmap.Set(index, false, name, superBlock.inode_bitmap_start_address);
}