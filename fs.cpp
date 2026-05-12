#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <ios>
#include <limits>
#include <memory>
#include <cctype>
#include <bits/stdc++.h>
#include "fs.h"



/**
 * Helper function used to parse an attribute of directory entry, like filename or size, first block etc.....
 * @param inputString any attribute in disk for a directory entry. for example ???????????.....filename.txt
 * @return parsed attribute
 */
std::string FS::dirParseAttr(std::string inputString)
{

    while (inputString.size() > 0)
    {
        std::string delimeter{inputString[0]};
        if (delimeter.compare(PLACE_HOLDER_CHAR) == 0)
        {
            inputString = inputString.substr(1);
        }
        else
            return inputString;
    }
    return "";
}




/**
 * helper function used in parsePath to backtrack to previous dir entry
 * @param visitedDirectories vector of visited directories from root
*/
void FS::backtrack(std::vector<std::string> visitedDirectories)
{
    while(std::string("/").compare(this->currentWorkingDir.file_name) != 0){
        this->cdHelper("..");
    }
    for(int i = visitedDirectories.size() - 1; i >= 0 ; i--){
        std::string directory = visitedDirectories.at(i);
        this->cdHelper(directory);
    }
}

/**
 * Helper function used to cd to a relative path.
 * @param dirpath relative path to directory
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 414 : Trying to cd to a file 
 * 
 * 404 : Trying to cd to a directory which does not exist
 * 
 */
int FS::cdHelper(std::string dirpath)
{
    std::unique_ptr<dir_entry> entry = this->findDirectoryEntry(dirpath); 
    
    if(dirpath.compare("..") == 0){
        if(entry == nullptr)
            return 0;
        dir_entry entryBefore = *entry;
        this->currentWorkingDir = entryBefore;
        this->loadNewDirectory();
        this->currentWorkingDir = entryBefore; 
        return 0;
    }
    
    if (entry == nullptr)
    {
        return FILE_NOT_FOUND;
    }
    else if(dirpath.compare("..") != 0){
        if(dirpath.compare("/") == 0){
            if(std::string(this->currentWorkingDir.file_name).compare("/") == 0)
                return 0;
            std::string filename = "/";
            this->currentWorkingDir = dir_entry((char *)filename.c_str(), 0, ROOT_BLOCK, TYPE_DIR, READ | WRITE | EXECUTE);
            this->loadNewDirectory();
            return 0;
        }
        else if(entry->type != TYPE_DIR){
            return ENTRY_IS_FILE;
        }
        this->currentWorkingDir = *entry; 
        this->loadNewDirectory();
    }
    return 0;
}

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    this->dirCount = 0;
    this->format();
}

FS::~FS()
{
}

// formats the disk, i.e., creates an empty file system
int FS::format()
{
    // std::cout << "FS::format()\n";
    this->dirCount = 0;
    this->fat[ROOT_BLOCK] = ROOT_BLOCK;
    this->fat[FAT_BLOCK] = FAT_BLOCK;
    for (int i = FAT_BLOCK + 1; i < NUMBER_OF_BLOCKS; i++)
    {
        this->fat[i] = FAT_FREE;
    }
    for (int i = 0; i < NUMBER_OF_BLOCKS; i++)
    {
        uint8_t empty[BLOCK_SIZE] = {0};
        this->disk.write(i, empty);
    }
    this->loadDirectory();
    return 0;
}


/**
 * Helper function used to get and set the next free block in the FAT-Table
 * @param blockBefore update block to point to next block in FAT-table / disk. Leave blank if getting a new block
 * @return -1 if not block is found otherwise returns the free block
 * 
 */
int FS::getFreeBlock(int blockBefore)
{
    for (int i = FAT_BLOCK + 1; i < NUMBER_OF_BLOCKS; i++)
    {
        if (this->fat[i] == FAT_FREE)
        {
            this->fat[i] = FAT_EOF;
            if (blockBefore != FAT_EOF)
            {
                this->fat[blockBefore] = i;
            }
            return i;
        }
    }
    return -1;
}


 /**
     * helper function used in create, cp, mv, mkdir to write dir_entry both in secondary memory and ram if dontAddEntry is true
     * @param entry dir_entry object, in current directory / dirEntries
     * @param dontAddEntry set to true if its in current directory otherwise false
     * @return Error Code. 
     * 
     * 0 : OK
     * 
     * -1 : no valid dir entry found
*/ 
int FS::writeDirectoryEntry(dir_entry entry, bool dontAddEntry)
{

    int index = this->getFreeDirEntryIndex();
    if (index == NO_VALID_INDEX)
        return NO_VALID_INDEX;
    std::string serializedEntry = entry.serializeEntry();
    std::string previousData = "";
    uint8_t buffer[BLOCK_SIZE];
    
    int currentDirectoryBlock = this->currentWorkingDir.first_blk;
    this->disk.read(currentDirectoryBlock, buffer);
    if(this->dirCount == 0){
        previousData = serializedEntry;
    }
    else{
        int amountToRead = this->dirCount * DIR_ENTRY_SIZE;
        previousData.assign((char *)buffer, amountToRead);
        previousData += serializedEntry;
    }

    this->safeString(previousData);
    this->disk.write(currentDirectoryBlock, (uint8_t *)previousData.c_str());
    if(dontAddEntry != true)
        this->dirEntries[index] = entry;
    this->dirCount++;
    return 0;
}



/**
 * Helper functions used to seach a directory entry in the current working directory
 * @param filepath relative path to a file
 * @return nullptr if not found, unique_ptr to dir_entry obj if found
 */
std::unique_ptr<dir_entry> FS::findDirectoryEntry(std::string filepath)
{
    bool found = false;
    int index = 0;

    if(currentWorkingDir.first_blk == ROOT_BLOCK && filepath == "..")
    {
        std::string filename = "/";
        dir_entry* root = new dir_entry((char *)filename.c_str(), 0, ROOT_BLOCK, TYPE_DIR, READ | WRITE | EXECUTE);
        return std::unique_ptr<dir_entry>(root);
    }
    else if(filepath == "..")
    {
        dir_entry entry = this->dirEntries[DIR_PARENT];
        return std::unique_ptr<dir_entry>(new dir_entry(entry));
    }

    while (!found && index < this->dirCount)
    {
        dir_entry entry = this->dirEntries[index++]; // Creates a reference
        std::string name = entry.file_name;
        if (filepath.compare(name) == 0)
        {
            return std::unique_ptr<dir_entry>(new dir_entry(entry)); // if not for the first step we return reference to an object which no longer exists
        }
    }
    return nullptr;
}


/**
     * helper function used in create, cp, mv, mkdir to write dir_entry both in secondary memory and ram if dontAddEntry is true
     * @param entry dir_entry object, in current directory / dirEntries
     * @param dontAddEntry set to true if its in current directory otherwise false
     * @return Error Code. 
     * 
     * 0 : OK
     * 
     * -1 : no valid dir entry found
*/ 
int FS::getFreeDirEntryIndex()
{

    int numberOfDirEntries = AMOUNT_OF_DIRS;
    if (this->dirCount != numberOfDirEntries)
    {
        for (int i = 0; i < numberOfDirEntries; i++)
        {
            std::string filename = "";
            filename = this->dirEntries[i].file_name;
            if (filename.compare("") == 0)
            {
                return i;
            }
        }
    }
    return NO_VALID_INDEX;
}

/**
     * helper function used in mv, rm, append, chmod to find file in disk. 
     * @param path the filename in disk without padding
     * @return index or Error Code. 
     * -1 : entry not found
*/ 
int FS::getDirIndex(std::string path)
{
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(this->currentWorkingDir.first_blk, buffer);
    std::string bufferString = "";
    bufferString.assign((char*)buffer, BLOCK_SIZE);
    int index = bufferString.find(this->addPadding(path));
    if(index == std::string::npos)
        return -1;
    return index;
}


// Wrapper around deserializeEntries used in format to initally load directory entries
void FS::loadDirectory() {

    this->deserializeEntries(ROOT_BLOCK);
    std::string filename = "/";
    dir_entry root = dir_entry((char *)filename.c_str(), 0, ROOT_BLOCK, TYPE_DIR, READ | WRITE | EXECUTE);
    this->currentWorkingDir = root;
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(ROOT_BLOCK, buffer);
}


// Wrapper arround deserializeEntries, which is more memorable. Used in all commands to get entries in current working directory
void FS::loadNewDirectory()
{
    this->deserializeEntries(this->currentWorkingDir.first_blk);
   
}

/**
 * Used to load and deserialize all the directory entries in disk for given disk block. 
 * @param block any block in the disk
 */
void FS::deserializeEntries(int block)
{
    dir_entry newEntry;
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(block, buffer);
    std::string bufferResult = "";
    bufferResult = (char*)buffer;
    if(bufferResult.size() >= BLOCK_SIZE)
        bufferResult.resize(BLOCK_SIZE);

    for(int i = 0; i < AMOUNT_OF_DIRS; i++){
        this->dirEntries[i] = dir_entry();
    }

    this->dirCount = 0;
    while (bufferResult.size() != 0)
    {
        std::string currentEntry = bufferResult.substr(0, DIR_ENTRY_SIZE);
        std::string filename = this->dirParseAttr(currentEntry.substr(0, FILENAME_CHARS));
        uint32_t fileSize = atoi(this->dirParseAttr(currentEntry.substr(FILENAME_CHARS, SIZE_CHARS)).c_str());
        uint16_t first_blk = atoi(this->dirParseAttr(currentEntry.substr(SIZE_CHARS + FILENAME_CHARS, FIRST_BLK_CHARS)).c_str());
        uint8_t type = atoi(this->dirParseAttr(currentEntry.substr(FIRST_BLK_CHARS + SIZE_CHARS + FILENAME_CHARS, TYPE_CHARS).c_str()).c_str());
        uint8_t access_rights = atoi(this->dirParseAttr(currentEntry.substr(TYPE_CHARS + FIRST_BLK_CHARS + SIZE_CHARS + FILENAME_CHARS, TYPE_CHARS)).c_str());

        dir_entry newEntry((char *)filename.c_str(), fileSize, first_blk, type, access_rights);
        this->dirEntries[this->dirCount++] = newEntry;
        bufferResult = bufferResult.substr(DIR_ENTRY_SIZE);
    }
}

/**
 * std::string puts garbage after the the end of the string. use this function to make the string safe for writing to disk
 * @param str string to padd with null-terminated characters
 */
void FS::safeString(std::string &str)
{
    int toPadd = BLOCK_SIZE - str.size();
    for (int i = 0; i < toPadd; i++)
        str += '\0';
}

/**
 * helper function used in all commands to check for a valid path 
 * @param path a path in any form 
 * @param fromChmod no need to set this one except form chmod. could not change permissions otherwise
 * @return either a pointer to tuple where
 * 
 * index 0 : filname or directory name of end string
 * 
 * index 1 : parent disk block
 * 
 * index 2 : parent directory name 
 */
std::unique_ptr<std::tuple<std::string, int, std::string>> FS::parsePath(std::string path, bool fromChmod)
{
    if (path.empty())
        return nullptr;


    if (path == "/")
        return std::unique_ptr<std::tuple<std::string, int, std::string>>(new std::tuple<std::string, int, std::string>("/", ROOT_BLOCK, "/"));

   
    if (path.find("//") != std::string::npos)
        return nullptr;

    
    dir_entry originalDir = this->currentWorkingDir;


    bool isAbsolute = (path[0] == '/');

    if (isAbsolute) {
        while (std::string(this->currentWorkingDir.file_name) != "/") {
            this->cdHelper("..");
        }
        path = path.substr(1); 
    }

   
    std::vector<std::string> parts;
    size_t pos = 0;
    while ((pos = path.find('/')) != std::string::npos) {
        std::string token = path.substr(0, pos);
        if (!token.empty())
            parts.push_back(token);
        path.erase(0, pos + 1);
    }
    if (!path.empty())
        parts.push_back(path);

    
    if (parts.empty()) {
        int block = this->currentWorkingDir.first_blk;
        std::string filename;

        if(isAbsolute)
            filename = "/";
        else
            filename = path;

        std::string parentFilename = this->currentWorkingDir.file_name;
        this->currentWorkingDir = originalDir;
        this->loadNewDirectory();
        return std::unique_ptr<std::tuple<std::string, int, std::string>>(new std::tuple<std::string, int, std::string>(filename, block, parentFilename));
    }

  
    for (size_t i = 0; i < parts.size() - 1; i++) {
        std::unique_ptr<dir_entry> entry = this->findDirectoryEntry(parts[i]);
        if (entry == nullptr || entry->type != TYPE_DIR) {
            this->currentWorkingDir = originalDir;
            this->loadNewDirectory();
            return nullptr;
        }
        dir_entry entryTest = *entry;
        if(!this->hasExecutePerm(entryTest)){
            this->currentWorkingDir = originalDir;
            this->loadNewDirectory();
            return nullptr;
        }

        this->cdHelper(parts[i]);
    }

    std::unique_ptr<dir_entry> entry = this->findDirectoryEntry(parts[parts.size() - 1]);
    dir_entry entryTest;
    bool flag = false;
    if(entry != nullptr){
        flag = true;
        entryTest = *entry;
    }
        
    if(entry != nullptr && !this->hasExecutePerm(entryTest, true) && entry->type == TYPE_DIR && !fromChmod){
        this->currentWorkingDir = originalDir;
        this->loadNewDirectory();
        return nullptr;
    }

    
    std::string filename = parts.back();
    int parentBlock = this->currentWorkingDir.first_blk;
    std::string parentFilename = this->currentWorkingDir.file_name;

    this->currentWorkingDir = originalDir;
    this->loadNewDirectory();

    if(filename.compare(parentFilename) == 0)
        return nullptr;
    return std::unique_ptr<std::tuple<std::string, int, std::string>>(new std::tuple<std::string, int, std::string>(filename, parentBlock, parentFilename));
}


/**
 * Used in create, mv, cp, getDirIndex to addding to a filename / directory name to later be stored on the disk
 * @param filepath a string of a filename or directory name
 * @return string of filename or directory name that can be stored withing a directory entry in disk
 */
std::string FS::addPadding(std::string filepath)
{
    std::string filenameInDisk = "";
    int amountOfPadding = FILENAME_CHARS - filepath.size();
    for (int i = 0; i < amountOfPadding; i++)
    {
        filenameInDisk += PLACE_HOLDER_CHAR;
    }
    filenameInDisk += filepath;
    return filenameInDisk;
}



/**
 * helper function used in create to see if filename already exists
 * @param filename unpadded filname to search for in disk
 * @return disk block if filename found, 0 if not found.
 */
int FS::getBlock(std::string filename)
{
    for (int block = FAT_BLOCK + 1; block < NUMBER_OF_BLOCKS; block++)
    {
        uint8_t buffer[BLOCK_SIZE];
        disk.read(block, buffer);
        std::string myString = "";
        myString.assign((char *)buffer, BLOCK_SIZE);
        std::string filenameInDisk = myString.substr(0, myString.find("\n"));
        if (filename.compare(filenameInDisk) == 0)
        {
            return block;
        }
    }

    return 0;
}


/**
 * create <filepath> creates a new file on the disk, the data content is
 * written on the following rows (ended with an empty row)
 * @param filepath any path wether its relative or absolute, where filepath <= 55
 * @return Error Code
 * 
 * 0  :  OK
 * 
 * 406 : Path is incorrect
 * 
 * 403 : Parent directory to the file being created is missing writing permission
 * 
 * 304 : Parent directory to the file being created, already contains entry with filename **filepath**
 * 
 * 413 : The argument sent in to filepath exceeds limit of 55 characters
 * 
 * -1 :  The direcortory block is full
 * 
*/
int FS::create(std::string filepath)
{
    // std::cout << "FS::create(" << filepath << ")\n";
    std::unique_ptr<std::tuple<std::string, int, std::string>> parsedPathPtr = this->parsePath(filepath);
    if(parsedPathPtr == nullptr)
        return WRONG_PATH_FORMAT;

    dir_entry dirBefore = this->currentWorkingDir;

    this->currentWorkingDir.first_blk = std::get<1>(*parsedPathPtr);
    this->loadNewDirectory();
    filepath = std::get<0>(*parsedPathPtr);

    if(!this->hasWritePerm(this->currentWorkingDir))
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return DIRECTORY_RESTRICTS_ACCESS;
    }

    if (this->getBlock(filepath)){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_EXISTS;
    }
        
    if (filepath.size() > FILENAME_CHARS - 1){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILENAME_TOO_LARGE;
    }
    if (this->dirCount == AMOUNT_OF_DIRS){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return NO_VALID_INDEX;
    }

    std::string userInput = "NONE";

    int currentSize = 0;
    int block = this->getFreeBlock();
    int firstBlock = block;

    std::string filenameInDisk = this->addPadding(filepath) + "\n";
    currentSize += filenameInDisk.size();
    std::string toAddString = "";

    toAddString += filenameInDisk;
    // for (int i = 0; i < BLOCK_SIZE; i++) {
    //    toAddString += "A";
    // }
    // currentSize = toAddString.size();
    int totalSizeOfFile = 0;

    while (std::getline(std::cin, userInput) && userInput.size() && !userInput.empty())
    {
        int previousSize = currentSize;
        currentSize += userInput.size() + 1;
        toAddString += userInput;
        toAddString += "\n";
        totalSizeOfFile += currentSize - previousSize;
        while (currentSize > BLOCK_SIZE)
        {
            std::string toWrite = toAddString.substr(0, BLOCK_SIZE);
            this->disk.write(block, (uint8_t *)toWrite.c_str());
            block = this->getFreeBlock(block);
            toAddString = toAddString.substr(BLOCK_SIZE);
            currentSize = toAddString.size();
        }
    }

    this->safeString(toAddString);
    this->disk.write(block, (uint8_t *)toAddString.c_str());

    dir_entry newEntry((char *)filepath.c_str(), totalSizeOfFile, firstBlock, TYPE_FILE, READ + WRITE);
    this->writeDirectoryEntry(newEntry);
    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();

    // std::cout << "File created successfully!\n";
    std::cin.clear();
    std::cin.sync();
    return 0;
}

/**
 * cat <filepath> reads the content of a file and prints it on the screen
 * @param filepath can be both relative and absolute, needs to exist
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : Path is incorrect
 * 
 * 404 : The file does not exist
 * 
 * 415 : Can't use cat on a directory
 * 
*/
int FS::cat(std::string filepath)
{
    // std::cout << "FS::cat(" << filepath << ")\n";
    std::unique_ptr<std::tuple<std::string, int, std::string>> dirParsed = this->parsePath(filepath);
    if(dirParsed == nullptr)
        return WRONG_PATH_FORMAT;
    dir_entry dirBefore = this->currentWorkingDir;
    this->currentWorkingDir.first_blk = std::get<1>(*dirParsed);
    this->loadNewDirectory();
    filepath = std::get<0>(*dirParsed);

    std::unique_ptr<dir_entry> entry = this->findDirectoryEntry(filepath);
    if(entry == nullptr){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }

    else if(entry->type == TYPE_DIR){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return ENTRY_IS_DIR;
    }

    dir_entry entryTest = *entry;
    if(!this->hasReadPerm(entryTest))
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PERMISSIONS;
    }

    int index = 0;
    while (index < AMOUNT_OF_DIRS)
    {
        dir_entry entry = this->dirEntries[index++];
        std::string filename = entry.file_name;
        if (filename.compare(filepath) == 0)
        {

            int block = entry.first_blk;
            uint8_t buffer[BLOCK_SIZE];
            this->disk.read(block, buffer);

            std::string content = "";
            content.assign((char *)buffer, BLOCK_SIZE);
            std::cout << content.substr(content.find("\n") + 1);

            while (block != FAT_EOF)
            {
                block = this->fat[block];
                if (block != FAT_EOF)
                {
                    this->disk.read(block, buffer);
                    content = "";
                    content.assign((char *)buffer, BLOCK_SIZE);
                    std::cout << content;
                }
            }
            return 0;
        }
    }
    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();
    return FILE_NOT_FOUND;
}

/**
 * ls lists the content in the currect directory (files and sub-directories)
 * @return Error Code
 *  
 * 0 : OK
 * 
 */ 
int FS::ls()
{

    // std::cout << "FS::ls()\n";

    int sizeOfNameString = 4;

    int count = 0;
    int index = 0;

    std::vector<dir_entry> entryVector;
    for(int i = 0; i < AMOUNT_OF_DIRS; i++){
        entryVector.push_back(this->dirEntries[i]);
    }

    for (int i = 1; i < AMOUNT_OF_DIRS; ++i) {
        dir_entry key = entryVector.at(i);
        int j = i - 1;

        while (j >= 0 && (strcmp(entryVector.at(j).file_name, key.file_name) > 0  && 
        std::string(entryVector.at(j).file_name).size() == std::string(key.file_name).size()  ||
        std::string(entryVector.at(j).file_name).size() > std::string(key.file_name).size() &&
        (strcmp(entryVector.at(j).file_name, key.file_name) > 0 ))) {
            entryVector.at(j + 1)= entryVector.at(j);
            j = j - 1;
        }
        entryVector.at(j + 1) = key;
    }


    if(this->dirCount == 1 && this->currentWorkingDir.first_blk != ROOT_BLOCK){
        std::cout << "name";
        for(int i = 0; i < FILENAME_CHARS; i++)
            std::cout << " ";
        std::cout << "type" << "\t"  "accessRights" << "\t" << "size" << "\n";
        return 0;
    }

    else if(this->currentWorkingDir.first_blk == ROOT_BLOCK && this->dirCount == 0){
        std::cout << "name";
        for(int i = 0; i < FILENAME_CHARS; i++)
            std::cout << " ";
        std::cout << "type" << "\t"  "accessRights" << "\t" << "size" << "\n";
        return 0;
    }

    while (index < AMOUNT_OF_DIRS)
    {
        dir_entry currentEntry = entryVector.at(index++);
        std::string filename = currentEntry.file_name;
        std::string size = std::to_string(currentEntry.size);
        std::string type = "";

        if(size.compare("0") == 0)
            size = "-";

        if(currentEntry.type == TYPE_FILE)
            type = "file";
        else
            type = "dir";

        
        if (filename.compare("") != 0){
                if(count == 0){
                    std::cout << "name";
                    for(int i = 0; i < FILENAME_CHARS - sizeOfNameString; i++)
                        std::cout << " ";
                    std::cout << "type" << "\t"  "accessRights" << "\t" << "size" << "\n";
                }
                if(count != 0 || this->currentWorkingDir.first_blk == ROOT_BLOCK){
                    int spaceFile = FILENAME_CHARS - filename.size();
                    std::string result = filename;
                    for(int i = 0; i < spaceFile; i++){
                        filename += " ";
                    }   
                    
                    std::string accessRightsString = "";
                    int accessRights = currentEntry.access_rights;
                    switch (accessRights)
                    {
                    case READ:
                        accessRightsString = "r-";
                        break;
                    case WRITE:
                        accessRightsString = "-w-";
                        break;
                    case EXECUTE:
                        accessRightsString = "--x";
                        break;
                    case READ + EXECUTE:
                        accessRightsString = "r-x";
                        break;
                    case READ + WRITE:
                        accessRightsString = "rw-";
                        break;
                    case WRITE + EXECUTE:
                        accessRightsString = "-wx";
                        break;   
                    case WRITE + EXECUTE + READ:
                        accessRightsString = "rwx";
                        break;  
                    default:
                        break;
                    }
                    std::cout << filename << type << "\t" << accessRightsString << "\t" << "\t" << size << "\n";
                }  
                count++;
        }
           
    }
    return 0;
}

/**
 * cp <sourcepath> <destpath> makes an exact copy of the file
 * @param sourcepath can both relative or absolute filepath, needs exists
 * @param destpath can both relative or absolute filepath, if it's a filename it cannot exist in the parent directory
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : Path is incorrect or missing permissions
 * 
 * 404 : sourcepath does not exist, or sourcepath is a directory and not a file
 * 
 * 304 : The File already exists in the destinatin parent directory
 * 
 * 413 : destpath filename is too large
 * 
 * -1 : Directory is full
 * 
*/
int FS::cp(std::string sourcepath, std::string destpath)
{
    // std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";

    std::unique_ptr<std::tuple<std::string, int, std::string>> sourcepathPtr = this->parsePath(sourcepath);
    std::unique_ptr<std::tuple<std::string, int, std::string>> destpathPtr = this->parsePath(destpath); 
    dir_entry dirBefore = this->currentWorkingDir;

    if(sourcepathPtr == nullptr || destpathPtr == nullptr)
        return WRONG_PATH_FORMAT;

    this->currentWorkingDir.first_blk = std::get<1>(*sourcepathPtr);
    this->loadNewDirectory();
    std::unique_ptr<dir_entry> source = this->findDirectoryEntry(std::get<0>(*sourcepathPtr));
    
    if (source == nullptr){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }

    if(source->type == TYPE_DIR)
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }
        
    dir_entry sourceEntry = *source;
    std::string sourceFilename = sourceEntry.file_name;

    this->currentWorkingDir.first_blk = std::get<1>(*destpathPtr);
    this->loadNewDirectory();
    dir_entry destParent = this->currentWorkingDir;
    std::unique_ptr<dir_entry> dest = this->findDirectoryEntry(std::get<0>(*destpathPtr));


    if (dest != nullptr && dest->type == TYPE_FILE){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_EXISTS;
    }

    if(dest != nullptr && dest->type == TYPE_DIR){
        dir_entry tempDir = this->currentWorkingDir;
        this->currentWorkingDir = *dest;
        this->loadNewDirectory();
        std::unique_ptr<dir_entry> sourceTest = this->findDirectoryEntry(std::get<0>(*sourcepathPtr));
        
        if(sourceTest != nullptr){
            this->currentWorkingDir = dirBefore;
            this->loadNewDirectory();
            return FILE_EXISTS;
        }

        this->currentWorkingDir = tempDir;
        this->loadNewDirectory();
    }
  

    dir_entry destTest;
    if(dest != nullptr)
        destTest = *dest;

    if(dest != nullptr && (!this->hasWritePerm(destTest) || !this->hasWritePerm(sourceEntry)))
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PERMISSIONS;
    }

    else if(!this->hasWritePerm(sourceEntry) || !this->hasWritePerm(destParent))
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PERMISSIONS;
    }


    std::string destFilename = "";
    bool isNullFlag = true;
    int parentBlock = 0;
    if(dest != nullptr && dest->type == TYPE_DIR){
        isNullFlag = false;
        destFilename = sourceFilename;
        parentBlock = dest->first_blk;
    }
    else if(dest == nullptr){
        destFilename = std::get<0>(*destpathPtr);
    }


    if (destFilename.size() > FILENAME_CHARS)
        return FILENAME_TOO_LARGE;

    if (this->dirCount >= AMOUNT_OF_DIRS)
        return NO_VALID_INDEX;


    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();
   

    dir_entry newEntry = sourceEntry;

    memset(newEntry.file_name, 0, sizeof(newEntry.file_name));
    strncpy(
        newEntry.file_name,
        destFilename.c_str(),
        sizeof(newEntry.file_name) - 1);

    int newBlock = this->getFreeBlock();
    int sourceFatBlock = sourceEntry.first_blk;
    newEntry.first_blk = newBlock;

    uint8_t buffer[BLOCK_SIZE];
    
    
    std::string newContent = "";

    newContent = this->addPadding(destFilename) + "\n";

    this->disk.read(sourceFatBlock, buffer);

    std::string content = "";
    content.assign((char *)buffer, BLOCK_SIZE);
    newContent += content.substr(content.find("\n") + 1);
    this->disk.write(newBlock, (uint8_t *)newContent.c_str());

    int previous = 0;
    int count = 0;
    sourceFatBlock = this->fat[sourceFatBlock];

    while (sourceFatBlock != FAT_EOF)
    {
        newBlock = this->getFreeBlock(newBlock);
        if(count == 0)
            newEntry.first_blk = newBlock;
        this->disk.read(sourceFatBlock, buffer);
        newContent.assign((char *)buffer, BLOCK_SIZE);
        this->disk.write(newBlock, (uint8_t *)newContent.c_str());
        previous = sourceFatBlock;
        sourceFatBlock = this->fat[sourceFatBlock];
        count++;
    }
    if (count > 0)
    {
        this->disk.write(newBlock, (uint8_t *)newContent.c_str());
    }
    
    int destBlock = 0;
    if(isNullFlag == true)
        this->currentWorkingDir.first_blk = std::get<1>(*destpathPtr);
    else
        this->currentWorkingDir.first_blk = parentBlock;
    this->loadNewDirectory();
    this->writeDirectoryEntry(newEntry);
    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();
    return 0;
}



/**
 * mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
 * or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
 * @param sourcepath path to a file relative or absolute
 * @param destpath path to a directory or the new name of the file in another directory. Cannot Exist
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : Path is incorrect or missing permissions. For example file is missing read or write or can't cd 
 * to directory due to missing execution permission
 * 
 * 404 : File in sourcepath does not exists
 * 
 * 304 : File exists in specified directory
 * 
 */
int FS::mv(std::string sourcepath, std::string destpath)
{
    // std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";

    std::unique_ptr<std::tuple<std::string, int, std::string>> sourcepathPtr = this->parsePath(sourcepath);
    std::unique_ptr<std::tuple<std::string, int, std::string>> destpathPtr = this->parsePath(destpath);

    if(sourcepathPtr == nullptr || destpathPtr == nullptr){
        return WRONG_PATH_FORMAT;
    }


    std::string sourcepathString = std::get<0>(*sourcepathPtr);
    std::string destpathString = std::get<0>(*destpathPtr);
    int destpathBlock = std::get<1>(*destpathPtr);
    bool flagNotExists = true;

    
    if(destpathBlock == this->currentWorkingDir.first_blk){
        std::unique_ptr<dir_entry> destTest = this->findDirectoryEntry(destpathString);
        if(destTest != nullptr && destTest->type != TYPE_DIR)
        {
            return FILE_EXISTS;
        }
        flagNotExists = false;
    }



    if(sourcepathString.compare(destpathString) == 0 && 
    std::get<1>(*destpathPtr) == std::get<1>(*sourcepathPtr))
    {
        return FILE_EXISTS;
    }

    dir_entry dirBefore = this->currentWorkingDir;
    this->currentWorkingDir.first_blk = std::get<1>(*sourcepathPtr);
    this->loadNewDirectory();
    
    
    std::string sourceFilename = std::get<0>(*sourcepathPtr);
    std::unique_ptr<dir_entry> sourceEntryPtr = this->findDirectoryEntry(sourceFilename);

    if(sourceEntryPtr == nullptr){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }

    dir_entry sourceEntry = *sourceEntryPtr;
    if(!this->hasWritePerm(sourceEntry) || !this->hasReadPerm(sourceEntry)){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PERMISSIONS;
    }

    std::string destFilename = std::get<0>(*destpathPtr);
    this->currentWorkingDir.first_blk = std::get<1>(*destpathPtr);
    this->loadNewDirectory();
    std::unique_ptr<dir_entry> destinationEntryPtr = this->findDirectoryEntry(destFilename);

    dir_entry destinationEntry; 
    bool isNullFlag = true;
    if(destinationEntryPtr != nullptr){
        isNullFlag = false;
        destinationEntry  = *destinationEntryPtr;
        std::string destinationEntryString = destinationEntry.file_name;

        if(sourceFilename.compare(destinationEntryString) == 0 &&
        sourceEntry.type == TYPE_FILE && destinationEntry.type == TYPE_FILE
        ){
            this->currentWorkingDir = dirBefore;
            this->loadNewDirectory();
            return FILE_EXISTS;
        }

        if(!this->hasWritePerm(destinationEntry)){
            this->currentWorkingDir = dirBefore;
            this->loadNewDirectory();
            return WRONG_PERMISSIONS;
        }

        dir_entry tempBefore = this->currentWorkingDir;
        this->currentWorkingDir = destinationEntry;
        this->loadNewDirectory();
        std::string sourceFilenameTest = sourceEntry.file_name;
        std::unique_ptr<dir_entry> entryInDest = this->findDirectoryEntry(sourceFilenameTest);

        if(entryInDest != nullptr){
            this->currentWorkingDir = dirBefore;
            this->loadNewDirectory();
            return WRONG_PERMISSIONS;
        }

        this->currentWorkingDir = tempBefore;
        this->loadNewDirectory();
    }
      
    if (!isNullFlag && destinationEntryPtr->type != TYPE_DIR)
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_EXISTS;
    }


    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();


 
    this->currentWorkingDir.first_blk = std::get<1>(*sourcepathPtr);
    this->loadNewDirectory();
    
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(this->currentWorkingDir.first_blk, buffer);
    std::string bufferString = "";
    bufferString.assign((char*)buffer, BLOCK_SIZE);

    int index = this->getDirIndex(sourceFilename);
    

    std::string dirEntryString = bufferString.substr(index, DIR_ENTRY_SIZE);
    std::string leftSide = bufferString.substr(0, index);
    bufferString = leftSide + bufferString.substr(index + DIR_ENTRY_SIZE);
    this->safeString(bufferString);
    this->disk.write(this->currentWorkingDir.first_blk, (uint8_t*)bufferString.c_str());


    if(!isNullFlag)
        this->currentWorkingDir.first_blk = destinationEntry.first_blk;
    else{
        memset(sourceEntry.file_name, 0, sizeof(sourceEntry.file_name));
        strncpy(
        sourceEntry.file_name,
        destpathString.c_str(),
        sizeof(sourceEntry.file_name) - 1);
        this->currentWorkingDir.first_blk = std::get<1>(*destpathPtr);
    }

        
    this->loadNewDirectory();

    this->writeDirectoryEntry(sourceEntry, true);
    
    this->disk.read(sourceEntry.first_blk, buffer);
    std::string firstBlockContent = "";
    firstBlockContent.assign((char *)buffer, BLOCK_SIZE);

    firstBlockContent = firstBlockContent.substr(firstBlockContent.find("\n"));

    std::string filenameInDisk = this->addPadding(destpathString);

    std::string newFirstBlockContent = filenameInDisk + firstBlockContent;
    this->disk.write(sourceEntry.first_blk, (uint8_t *)newFirstBlockContent.c_str());

    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();

    return 0;
}

/**
 * rm <filepath> removes / deletes the file <filepath>
 * @param filepath Can be relative or absolute. Needs to exists, if directory, needs to be empty aswell
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : Path is incorrect or missing write permission in directory
 * 
 * 404 : The directory entry specified by filepath does not exist
 * 
 * 405 : The directory is not empty, tried to delete current working directory.
 * 
 */
int FS::rm(std::string filepath)
{
    // std::cout << "FS::rm(" << filepath << ")\n";

    std::unique_ptr<std::tuple<std::string, int, std::string>> filepathPtr = this->parsePath(filepath);
    if(filepathPtr == nullptr) 
        return WRONG_PATH_FORMAT;
    
    std::string entryStringName = std::get<0>(*filepathPtr);
    dir_entry dirBefore = this->currentWorkingDir;
    this->currentWorkingDir.first_blk = std::get<1>(*filepathPtr);
    this->loadNewDirectory();

    std::unique_ptr<dir_entry> entry = this->findDirectoryEntry(entryStringName);

    if (entry == nullptr){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }
    if(entry->type == TYPE_DIR){
        if(std::string(dirBefore.file_name).compare(entryStringName) == 0){
            this->currentWorkingDir = dirBefore;
            this->loadNewDirectory();
            return ENTRY_CANNOT_BE_DELETED; 
        }
        dir_entry temp = this->currentWorkingDir;
        this->currentWorkingDir = *entry;
        this->loadNewDirectory();
        if(entry->first_blk == ROOT_BLOCK || this->dirCount > 1)
        {
            this->currentWorkingDir = dirBefore;
            this->loadNewDirectory();
            return ENTRY_CANNOT_BE_DELETED; 
        }
        this->currentWorkingDir = temp;
        this->loadNewDirectory();
    }

    if(!this->hasWritePerm(this->currentWorkingDir)){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PERMISSIONS;
    }


    
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(this->currentWorkingDir.first_blk, buffer); 
    int index = this->getDirIndex(entryStringName);
    std::string bufferString = "";
    bufferString.assign((char*)buffer, BLOCK_SIZE);
    std::string serializedFile = bufferString.substr(index, DIR_ENTRY_SIZE);
    std::string serializedBlock = serializedFile.substr(FILENAME_CHARS + SIZE_CHARS, FIRST_BLK_CHARS).c_str();
    int blockDisk = atoi((this->dirParseAttr(serializedBlock)).c_str());

    this->disk.read(blockDisk, buffer); 
    std::string testOfFile = (char*)buffer;
    if(testOfFile.size() > DIR_ENTRY_SIZE*2){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return ENTRY_CANNOT_BE_DELETED; 
    }

    std::string rightString = bufferString.substr(index + DIR_ENTRY_SIZE);
    std::string leftString = bufferString.substr(0, index);
    bufferString = leftString + rightString;
    this->safeString(bufferString);
    this->disk.write(this->currentWorkingDir.first_blk, (uint8_t*)bufferString.c_str());
    
    
    int block = entry->first_blk;
    int previous = block;
    while (block != FAT_EOF)
    {
        std::string emptyContent = "";
        this->safeString(emptyContent);
        this->disk.write(block, (uint8_t *)emptyContent.c_str());
        previous = block;
        block = this->fat[block];
        this->fat[previous] = FAT_FREE;
    }
    this->fat[previous] = FAT_FREE;
    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();

    return 0;
}

/**
 * append <filepath1> <filepath2> appends the contents of file <filepath1> to
 * the end of file <filepath2>. The file <filepath1> is unchanged.dir_entry
 * @param filepath1 Can be relative or absolute needs to exist and can't be a file
 * @param filepath2 Can be relative or absolute needs to exist and can't be a file
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : Path is incorrect or missing sufficient permissions
 * 
 * 404 : filepath1 or filepath2 does not exist
 * 
 * 415 : filepath1 or filepath2 is not a file
 * 
*/
int FS::append(std::string filepath1, std::string filepath2)
{
    // std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    std::unique_ptr<std::tuple<std::string, int, std::string>> filepath1Ptr = this->parsePath(filepath1);
    std::unique_ptr<std::tuple<std::string, int, std::string>> filepath2Ptr = this->parsePath(filepath2);

    if(filepath1Ptr == nullptr || filepath2Ptr == nullptr)
        return WRONG_PATH_FORMAT;
    
    dir_entry dirBefore = this->currentWorkingDir;
    std::string filepath1String = std::get<0>(*filepath1Ptr);
    std::string filepath2String = std::get<0>(*filepath2Ptr);
    
    this->currentWorkingDir.first_blk = std::get<1>(*filepath1Ptr); 
    this->loadNewDirectory();
    std::unique_ptr<dir_entry> src = this->findDirectoryEntry(filepath1String);

    if(src == nullptr)
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }

    dir_entry srcEntry = *src;

    this->currentWorkingDir.first_blk = std::get<1>(*filepath2Ptr); 
    this->loadNewDirectory();
    std::unique_ptr<dir_entry> dest = this->findDirectoryEntry(filepath2String);

    if (dest == nullptr)
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }
    dir_entry destEntry = *dest;

    if(!this->hasWritePerm(destEntry) ||
    !this->hasReadPerm(srcEntry) 
    ){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PERMISSIONS;
    }

    
    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();

    if(destEntry.type != TYPE_FILE || srcEntry.type != TYPE_FILE)
    {
        return ENTRY_IS_DIR;
    }
    uint8_t buffer[BLOCK_SIZE];
    int destParentBlock = std::get<1>(*filepath2Ptr);

    this->disk.read(destParentBlock, buffer);
    std::string dirBufferString = "";
    dirBufferString.assign((char *)buffer, BLOCK_SIZE);
    int dirIndex = this->getDirIndex(filepath2String);
    int newSize = srcEntry.size + destEntry.size;
    std::string newSizeString = std::to_string(newSize);

    int sizeOfString = newSizeString.size();
    for(int i = 0; i < SIZE_CHARS - sizeOfString; i++)
    {
        newSizeString = PLACE_HOLDER_CHAR + newSizeString;
    }

    dirBufferString.replace(dirIndex + FILENAME_CHARS, SIZE_CHARS, newSizeString);
    this->safeString(dirBufferString);
    this->disk.write(destParentBlock, (uint8_t*)dirBufferString.c_str());
    this->disk.read(ROOT_BLOCK, buffer);
    int srcBlock = srcEntry.first_blk;
    int destBlock = destEntry.first_blk;
    int destPrevBlock = destBlock;

    // dest traverse till end of file
    while (destBlock != FAT_EOF)
    {
        destPrevBlock = destBlock;
        destBlock = this->fat[destBlock];
    }
    destBlock = destPrevBlock;

    std::string toAdd = "";
    int count = 0;
    while (srcBlock != FAT_EOF)
    {
        std::string contentSrc = "";
        this->disk.read(srcBlock, buffer);
        contentSrc.assign((char *)buffer, BLOCK_SIZE);

        int indexOfNull = contentSrc.find('\0');
        if (indexOfNull != std::string::npos)
            contentSrc = contentSrc.substr(0, indexOfNull);
        contentSrc = toAdd + contentSrc;

        if (count == 0)
        {
            this->disk.read(destBlock, buffer);
            toAdd += (char *)buffer;
            contentSrc = contentSrc.substr(contentSrc.find("\n") + 1);
            toAdd += contentSrc;

            if (toAdd.size() > BLOCK_SIZE)
            {
                std::string toAddBlock = toAdd.substr(0, BLOCK_SIZE);
                this->disk.write(destBlock, (uint8_t *)toAddBlock.c_str());
                toAdd = toAdd.substr(BLOCK_SIZE);
            }
            else
            {
                this->safeString(toAdd);
                this->disk.write(destBlock, (uint8_t *)toAdd.c_str());
                toAdd = "";
            }
        }

        else
        {
            this->safeString(contentSrc);
            destBlock = this->getFreeBlock(destBlock);
            this->disk.write(destBlock, (uint8_t *)contentSrc.c_str());
        }

        srcBlock = this->fat[srcBlock];
        count++;
    }
    this->loadNewDirectory();
    return 0;
}

/** 
 * mkdir <dirpath> creates a new sub-directory with the name <dirpath>
 * in the current directory
 * @param dirpath Can be absolute or relative path for new directory, cannot not exist
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : Path is incorrect 
 * 
 * 413 : directory name specified by dirpath exceeds limit of 55
 * 
 * 304 : The directory already exists
 * 
 */
int FS::mkdir(std::string dirpath)
{
    // std::cout << "FS::mkdir(" << dirpath << ")\n";
    std::unique_ptr<std::tuple<std::string, int, std::string>> parsedPathPtr = this->parsePath(dirpath);

    if(parsedPathPtr == nullptr)
        return WRONG_PATH_FORMAT;
    if(std::get<0>(*parsedPathPtr).size() > 55)
        return FILENAME_TOO_LARGE;

    std::string parentFilename = std::get<2>(*parsedPathPtr);
    dir_entry dirBefore = this->currentWorkingDir;
    dirpath = std::get<0>(*parsedPathPtr);
    this->currentWorkingDir.first_blk = std::get<1>(*parsedPathPtr);
    
    memset(this->currentWorkingDir.file_name, 0, sizeof(this->currentWorkingDir.file_name));
    strncpy(
        this->currentWorkingDir.file_name,
        (char*)parentFilename.c_str(),
        sizeof(this->currentWorkingDir.file_name) - 1); 
    
    this->loadNewDirectory();

    if(!this->hasExecutePerm(this->currentWorkingDir) || 
    !this->hasWritePerm(this->currentWorkingDir)){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PERMISSIONS;
    }

    std::string filename = std::get<0>(*parsedPathPtr);
    std::unique_ptr<dir_entry> entryTest = this->findDirectoryEntry(filename);

    if(entryTest != nullptr)
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_EXISTS;
    }

   
    int newDirBlock = this->getFreeBlock();
    dir_entry newDirEntry((char *)dirpath.c_str(), 0, newDirBlock, TYPE_DIR, READ | WRITE | EXECUTE);
    this->writeDirectoryEntry(newDirEntry);

    std::string parrent = this->currentWorkingDir.file_name;
    int parentBlock = this->currentWorkingDir.first_blk;
    dir_entry parentDirEntry((char*)parrent.c_str(), 0, parentBlock, TYPE_DIR, READ | WRITE | EXECUTE);
    this->currentWorkingDir = newDirEntry;

    int previousDirCount = this->dirCount;
    this->dirCount = 0;
    this->writeDirectoryEntry(parentDirEntry, true);
    this->currentWorkingDir = dirBefore;
    this->dirCount = previousDirCount;
    this->loadNewDirectory();

    return 0;
}

/**
 * cd <dirpath> changes the current (working) directory to the directory named <dirpath>
 * @param dirpath can be absolute path or relative path to a directory
 * @param muteCall for debugging purposes, prints to terminal if function is called
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : Path is incorrect or a directory missing execute permissions in dirpath
 * 
 * 404 : The directory does not exists
 * 
 * 304 : dirpath is file
 * 
 */
int FS::cd(std::string dirpath, bool muteCall)
{   
    // if(!muteCall)
    //     std::cout << "FS::cd(" << dirpath << ")\n";
    std::unique_ptr<std::tuple<std::string, int, std::string>> parsedPathPtr = this->parsePath(dirpath);
    dir_entry dirBefore = this->currentWorkingDir;
   
    if(parsedPathPtr == nullptr){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return WRONG_PATH_FORMAT;
    }
        
    this->currentWorkingDir.first_blk = std::get<1>(*parsedPathPtr);
    this->loadNewDirectory();
    std::string stringAtEnd = std::get<0>(*parsedPathPtr);
    std::unique_ptr<dir_entry> entry = this->findDirectoryEntry(stringAtEnd);
    

    if(entry == nullptr){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }

    if(entry->type == TYPE_FILE){
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_EXISTS;
    }

    this->currentWorkingDir = *entry;
    this->loadNewDirectory();
    return 0;
}


/**
 * Use as pwd in terminal to print current working directory as absolute path
 * @return Error Code
 * 0 : OK
 */
int FS::pwd()
{
    // std::cout << "FS::pwd()\n";
    if(std::string(this->currentWorkingDir.file_name).compare("/") == 0)
    {
        std::cout << "/" << "\n";
        return 0;
    } 

 
    std::string path = this->currentWorkingDir.file_name;
    dir_entry dirBefore = this->currentWorkingDir;
    while(this->currentWorkingDir.first_blk != ROOT_BLOCK){
        this->currentWorkingDir = this->dirEntries[DIR_PARENT];
        this->loadNewDirectory();
        std::string currentFilename = this->currentWorkingDir.file_name;
        if(this->currentWorkingDir.first_blk != ROOT_BLOCK)
            path = currentFilename + "/" + path;
    }

    this->currentWorkingDir = dirBefore;
    this->loadNewDirectory();
    std::cout << "/" << path << "\n";
    return 0;
}

/**
 * chmod <accessrights> <filepath> changes the access rights for the
 * file <filepath> to <accessrights>.
 * @param accessrights Number like 1, 2, 3, 4.....etc. View fs.h for table with different possibillities
 * @param filepath Can be absolute or relative path for a file or directory
 * @return Error Code
 * 
 * 0 : OK
 * 
 * 406 : accessrights argument is either not a digit or wrong format. Between 1 - 7 Or path format is wrong
 * 
 * 404 : Directory or file does not exist
 *  
 */
int FS::chmod(std::string accessrights, std::string filepath)
{
    // std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    
    int accessNum = 0;
    if(accessrights.size() != 1){
        return WRONG_PERMISSION_FORMAT;
    }  

    char character = accessrights.at(0);
    if(isdigit(character))
    {
        std::string charactarString(1, character);
        accessNum = atoi(charactarString.c_str());
    }
    else{
        return WRONG_PERMISSION_FORMAT;
    }

    std::unique_ptr<std::tuple<std::string, int, std::string>> filepathPtr = this->parsePath(filepath, true);
    if(filepathPtr == nullptr)
        return WRONG_PATH_FORMAT;

    std::string filename = std::get<0>(*filepathPtr);
    int parentBlock = std::get<1>(*filepathPtr);

    dir_entry dirBefore = this->currentWorkingDir;
    this->currentWorkingDir.first_blk = parentBlock;
    this->loadNewDirectory();

    std::unique_ptr<dir_entry> entryPtr = this->findDirectoryEntry(filename);
    
    if(entryPtr == nullptr)
    {
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();
        return FILE_NOT_FOUND;
    }

    dir_entry entry = *entryPtr;
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(parentBlock, buffer);
    std::string bufferString = "";
    bufferString.assign((char*)buffer, BLOCK_SIZE);
    int index = this->getDirIndex(filename);
    
    bufferString.replace(index + FILENAME_CHARS + SIZE_CHARS + FIRST_BLK_CHARS + TYPE_CHARS, 1, std::to_string(accessNum));
    this->safeString(bufferString);
    this->disk.write(parentBlock, (uint8_t*)bufferString.c_str());
    if(parentBlock == this->currentWorkingDir.first_blk)
        dirBefore.access_rights = accessNum;
    this->currentWorkingDir = dirBefore;
    loadNewDirectory();

    return 0;
}


/**
 * Helper function used to check for write permission
 * @param entry dir_entry object
 * @param mutecall set to true to remove printing missing permission to terminal
 * @return true if has write permission false if not
 */
bool FS::hasWritePerm(dir_entry entry, bool muteCall)
{
    int accessRights = entry.access_rights;
    switch (accessRights)
    {
    case WRITE: 
        return true;
        break;
    case WRITE + READ: 
        return true;
        break;
    case WRITE + EXECUTE:
        return true;
        break;
    case WRITE + READ + EXECUTE:
        return true;
        break;
    default:
        if(!muteCall)
            std::cout << "Missing Write Permission!" << "\n";
        return false;
        break;
    }
}


/**
 * Helper function used to check for execute permission
 * @param entry dir_entry object
 * @param mutecall set to true to remove printing missing permission to terminal
 * @return true if has execute permission false if not
 */
bool FS::hasExecutePerm(dir_entry entry, bool muteCall)
{
    int accessRights = entry.access_rights;
    switch (accessRights)
    {
    case EXECUTE: 
        return true;
        break;
    case EXECUTE + READ: 
        return true;
        break;
    case EXECUTE + WRITE:
        return true;
        break;
    case WRITE + READ + EXECUTE:
        return true;
        break;
    default:
        if(!muteCall)
            std::cout << "Missing Execute Permission!" << "\n";
        return false;
        break;
    }
}

/**
 * Helper function used to check for read permission
 * @param entry dir_entry object
 * @param mutecall set to true to remove printing missing permission to terminal
 * @return true if has read permission false if not
 */
bool FS::hasReadPerm(dir_entry entry, bool muteCall)
{
    int accessRights = entry.access_rights;
    if(accessRights >= 4)
        return true;
    if(!muteCall)
        std::cout << "Missing Read Permission!" << "\n";
    return false;
}


/**
 * dir_entry constructor
 * @param file_name can't execeed 55 character
 * @param size size of all contents filename block, directories have size 0
 * @param first_blk The first block of where the file begins or where directory block is
 * @param type TYPE_DIR or TYPE_FILE 
 * @param access_rights see table in fs.h to see all options
 * 
 */
dir_entry::dir_entry(char *file_name, uint32_t size, uint16_t first_blk, uint8_t type, uint8_t access_rights)
{
    memset(this->file_name, 0, sizeof(this->file_name));
    strncpy(
        this->file_name,
        file_name,
        sizeof(this->file_name) - 1);

    this->size = size;
    this->first_blk = first_blk;
    this->type = type;
    this->access_rights = access_rights;
}

dir_entry::dir_entry()
{
    std::string filenameStr = "";
    memset(this->file_name, 0, sizeof(this->file_name));
    strncpy(
        this->file_name,
        filenameStr.c_str(),
        sizeof(this->file_name) - 1);

    this->size = 0;
    this->first_blk = 0;
    this->type = 0;
    this->access_rights = 1;
}

dir_entry &dir_entry::operator=(const dir_entry &other)
{
    // TODO: insert return statement here
    memset(this->file_name, 0, sizeof(this->file_name));
    strncpy(
        this->file_name,
        other.file_name,
        sizeof(other.file_name) - 1);
    this->size = other.size;
    this->first_blk = other.first_blk;
    this->type = other.type;
    this->access_rights = other.access_rights;
    return *this;
}

/**
 * Helper function used to serialaze entry to be written to disk.
 * Since block size is 4KB entries all entries in the disk need to be 64 characters
 * if a field does not take up all space its padded with PLACE_HOLDER_CHAR value.
 * 
 */
std::string dir_entry::serializeEntry()
{
    std::string toReturn = "";
    std::string filenameString = this->file_name;
    std::string placeHolder = PLACE_HOLDER_CHAR;
    int charsCap = (sizeof(this->file_name) / sizeof(char));

    for (int i = 0; i < charsCap - filenameString.size(); i++)
        toReturn += placeHolder;
    toReturn += this->file_name;

    std::string sizeString = std::to_string(this->size);
    int sizeOfSizeString = sizeString.size();
    int maxChars = sizeof(this->size);
    for (int i = 0; i < maxChars - sizeOfSizeString; i++)
        toReturn += placeHolder;
    toReturn += sizeString;

    std::string blkString = std::to_string(this->first_blk);
    int sizeOfBlkString = blkString.size();
    int maxBlkChars = sizeof(this->first_blk);

    for (int i = 0; i < maxBlkChars - sizeOfBlkString; i++)
        toReturn += placeHolder;
    toReturn += blkString;
    toReturn += std::to_string(this->type) + std::to_string(this->access_rights);

    return toReturn;
}
