
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <ios>
#include <limits>
#include <vector>

#include "fs.h"

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

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    this->dirCount = 0;
    this->format();
    this->loadDirectory();
}

FS::~FS()
{
}

// formats the disk, i.e., creates an empty file system
// FORMAT WORKS!!!
int FS::format()
{
    std::cout << "FS::format()\n";
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
    return 0;
}

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

void FS::testArgs(uint8_t *buffer)
{
    std::string test = (char *)buffer;
    std::cout << test << "\n";

    uint8_t anotherTest[BLOCK_SIZE];
    for (int i = 0; i < test.size(); i++)
    {
        anotherTest[i] = test[i];
    }
    anotherTest[test.size()] = '\0';
    std::string sometingElse = (char *)anotherTest;
    std::cout << sometingElse << "\n";
}

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

dir_entry *FS::findDirectoryEntry(std::string filepath)
{
    bool found = false;
    int index = 0;
    while (!found && index < this->dirCount)
    {
        dir_entry &entry = this->dirEntries[index++]; // Creates a reference
        std::string name = entry.file_name;
        if (filepath.compare(name) == 0)
        {
            return &entry; // if not for the first step we return reference to an object which no longer exists
        }
    }
    return nullptr;
}

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

void FS::loadDirectory() {

    this->deserializeEntries(ROOT_BLOCK);
    if (this->dirCount == 0) {
        std::string filename = "/";
        dir_entry root = dir_entry((char *)filename.c_str(), 0, ROOT_BLOCK, TYPE_DIR, READ | WRITE | EXECUTE);
        this->writeDirectoryEntry(root);
    }
    this->currentWorkingDir = this->dirEntries[0];
}


//Loads new dirEntry
void FS::loadNewDirectory()
{
    this->deserializeEntries(this->currentWorkingDir.first_blk);
   
}

void FS::deserializeEntries(int block)
{
    dir_entry newEntry;
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(block, buffer);
    std::string bufferResult = "";
    bufferResult = (char *)buffer;

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

void FS::printer(int block)
{
    if (block == FAT_EOF)
    {
        std::cout << "END OF FILE!\n";
        return;
    }
    uint8_t newBuffer[BLOCK_SIZE];

    while (block != FAT_EOF && block != FAT_FREE)
    {
        std::cout << "___________________________\n";
        this->disk.read(block, newBuffer);
        std::string result = "";
        result.assign((char *)newBuffer, BLOCK_SIZE);
        std::cout << result << "\n";
        std::cout << "___________________________\n";
        block = this->fat[block];
    }
}

void FS::safeString(std::string &str)
{
    int toPadd = BLOCK_SIZE - str.size();
    for (int i = 0; i < toPadd; i++)
        str += '\0';
}

std::string FS::addPadding(std::string filepath)
{
    std::string filenameInDisk = "";
    int amountOfPadding = FILENAME_CHARS - (filepath.size() + 1);
    for (int i = 0; i < amountOfPadding; i++)
    {
        filenameInDisk += PLACE_HOLDER_CHAR;
    }
    filenameInDisk += filepath;
    return filenameInDisk;
}

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

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
// CHANGE LATER
// We are not checking length of filenames.
int FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";

    if (this->getBlock(filepath))
        return FILE_EXISTS;
    if (filepath.size() > FILENAME_CHARS)
        return FILENAME_TOO_LARGE;
    if (this->dirCount == AMOUNT_OF_DIRS)
        return NO_VALID_INDEX;

    std::string userInput = "NONE";

    int currentSize = 0;
    int block = this->getFreeBlock();
    int firstBlock = block;

    std::string filenameInDisk = this->addPadding(filepath) + "\n";
    currentSize += filenameInDisk.size();
    std::string toAddString = "";

    toAddString += filenameInDisk;
    // // for (int i = 0; i < BLOCK_SIZE; i++) {
    // //    toAddString += "A";
    // // }
    // // currentSize = toAddString.size();
    int totalSizeOfFile = 0;

    while (std::getline(std::cin, userInput) && userInput.size() && !userInput.empty())
    {
        int previousSize = currentSize;
        currentSize += userInput.size();
        toAddString += userInput;
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

    dir_entry newEntry((char *)filepath.c_str(), totalSizeOfFile, firstBlock, TYPE_FILE, 1);
    this->writeDirectoryEntry(newEntry);

    std::cout << "File created successfully!\n";
    std::cin.clear();
    std::cin.sync();
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    dir_entry* entry = this->findDirectoryEntry(filepath);
    if(entry == nullptr)
        return FILE_NOT_FOUND;
    else if(entry->type == TYPE_DIR)
        return ENTRY_IS_DIR;

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
            std::cout << content.substr(content.find("\n") + 1) << "\n";

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

    return FILE_NOT_FOUND;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{

    std::cout << "FS::ls()\n";
    std::cout << "name" << "";
    int sizeOfNameString = 4;
    for(int i = 0; i < FILENAME_CHARS - sizeOfNameString; i++)
        std::cout << " ";
    std::cout << "type" << "\t" << "size" << "\n";
    int index = 0;
    while (index < AMOUNT_OF_DIRS)
    {
        dir_entry currentEntry = this->dirEntries[index++];
        std::string filename = currentEntry.file_name;
        std::string size = std::to_string(currentEntry.size);
        std::string type = "";
        if(currentEntry.type == TYPE_FILE)
            type = "file";
        else
            type = "dir";
        if (filename.compare("") != 0){
            if(filename.compare(this->currentWorkingDir.file_name) == 0)
                filename = ".";
            int spaceFile = FILENAME_CHARS - filename.size();
            std::string result = filename;
            for(int i = 0; i < spaceFile; i++){
                filename += " ";
            }   
            std::cout << filename << type << "\t" << size << "\n";
        }
           
    }
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// MIGHT BE A PROBLEM WITH COPYING TO SAME DIR
int FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    dir_entry *source = this->findDirectoryEntry(sourcepath);
    dir_entry *dest = this->findDirectoryEntry(destpath);
    dir_entry sourceEntry = *source; 

    if (source == nullptr)
        return FILE_NOT_FOUND;
    if (dest != nullptr){
        if(dest->type = TYPE_DIR && destpath.compare(this->currentWorkingDir.file_name) != 0)
        {
            dir_entry previousEntry = this->currentWorkingDir;
            dir_entry newEntry = *dest;
            this->cd(destpath, true);
            dir_entry *sourceEntryInDest = this->findDirectoryEntry(sourcepath);
            if(sourceEntryInDest != nullptr)
                return FILE_EXISTS;
            this->writeDirectoryEntry(sourceEntry, true);
            this->currentWorkingDir = previousEntry;
            this->loadNewDirectory();
            
            return 0;
        }
        return FILE_EXISTS;
    }
        
    if (destpath.size() > FILENAME_CHARS)
        return FILENAME_TOO_LARGE;

    if (this->dirCount > AMOUNT_OF_DIRS)
        return NO_VALID_INDEX;

    dir_entry newEntry = sourceEntry;

    memset(newEntry.file_name, 0, sizeof(newEntry.file_name));
    strncpy(
        newEntry.file_name,
        destpath.c_str(),
        sizeof(newEntry.file_name) - 1);

    int newBlock = this->getFreeBlock();
    int sourceFatBlock = sourceEntry.first_blk;
    newEntry.first_blk = newBlock;

    uint8_t buffer[BLOCK_SIZE];

    std::string newContent = this->addPadding(destpath) + "\n";
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

    this->writeDirectoryEntry(newEntry);
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)

//LEFT OF HERE!!!!!!!!
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    dir_entry * destinationEntryPtr = this->findDirectoryEntry(destpath);
    dir_entry * sourceEntryPtr = this->findDirectoryEntry(sourcepath);
    if (destinationEntryPtr != nullptr && destinationEntryPtr->type != TYPE_DIR)
    {
        return FILE_EXISTS;
    }
    if (sourceEntryPtr == nullptr)
    {
        return FILE_NOT_FOUND;
    }
    if(destpath.compare(this->currentWorkingDir.file_name) == 0)
        return FILE_EXISTS;

    if(destinationEntryPtr->type == TYPE_DIR)
    {   
        dir_entry destinationEntry = *destinationEntryPtr;
        dir_entry sourceEntry = *sourceEntryPtr;
        uint8_t buffer[BLOCK_SIZE];
        this->disk.read(this->currentWorkingDir.first_blk, buffer);
        std::string bufferString = "";
        bufferString.assign((char*)buffer, BLOCK_SIZE);
        int index = this->getDirIndex(sourcepath);
        
        std::string dirEntryString = bufferString.substr(index, DIR_ENTRY_SIZE - 1);
        std::string leftSide = bufferString.substr(0, index - 1);
        bufferString = leftSide + bufferString.substr(index + DIR_ENTRY_SIZE - 1);
        this->safeString(bufferString);
        this->disk.write(this->currentWorkingDir.first_blk, buffer);


        int previousDirCount = this->dirCount;
        dir_entry dirBefore = this->currentWorkingDir;
        this->cd(destpath);
        index = this->getDirIndex(sourcepath);
        if(index == -1)
            this->writeDirectoryEntry(sourceEntry, true);
        this->currentWorkingDir = dirBefore;
        this->loadNewDirectory();

        if(index == -1)
            return FILE_EXISTS;
        return 0;
    }
    
    dir_entry &sourceEntry = *sourceEntryPtr;

    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(sourceEntry.first_blk, buffer);
    std::string firstBlockContent = "";
    firstBlockContent.assign((char *)buffer, BLOCK_SIZE);

    firstBlockContent = firstBlockContent.substr(firstBlockContent.find("\n"));

    std::string filenameInDisk = this->addPadding(destpath);

    std::string newFirstBlockContent = filenameInDisk + firstBlockContent;
    this->disk.write(sourceEntry.first_blk, (uint8_t *)newFirstBlockContent.c_str());

    memset(sourceEntry.file_name, 0, sizeof(sourceEntry.file_name));
    strncpy(
        sourceEntry.file_name,
        destpath.c_str(),
        sizeof(sourceEntry.file_name) - 1);

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    dir_entry *entry = this->findDirectoryEntry(filepath);
    if (entry == nullptr)
        return FILE_NOT_FOUND;
    if(entry->type == TYPE_DIR){
        if(std::string(this->currentWorkingDir.file_name).compare(filepath) == 0)
            return ENTRY_CANNOT_DELETED; 
        

        uint8_t buffer[BLOCK_SIZE];
        this->disk.read(this->currentWorkingDir.first_blk, buffer); 
        int index = this->getDirIndex(filepath);
        std::string bufferString = "";
        bufferString.assign((char*)buffer, BLOCK_SIZE);
        std::string serializedFile = bufferString.substr(index, DIR_ENTRY_SIZE - 1);
        std::string serializedBlock = serializedFile.substr(FILENAME_CHARS + SIZE_CHARS - 1, FIRST_BLK_CHARS).c_str();
        int block = atoi((this->dirParseAttr(serializedBlock)).c_str());
        
        this->disk.read(block, buffer); 
        std::string testOfFile = (char*)buffer;
        if(testOfFile.size() > DIR_ENTRY_SIZE*2)
            return ENTRY_CANNOT_DELETED;

        std::string rightString = bufferString.substr(index + DIR_ENTRY_SIZE - 1);
        std::string leftString = bufferString.substr(index - 1);
        bufferString = leftString + rightString;
        this->safeString(bufferString);
        this->disk.write(this->currentWorkingDir.first_blk, (uint8_t*)bufferString.c_str());
        *entry = dir_entry();
        this->dirCount--;
        return 0;
    }
    
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

    dir_entry emptyEntry;
    *entry = emptyEntry;
    this->dirCount--;

    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.dir_entry

// ISSUE Possibly with writing blocks over range or something due to added \n character
int FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    dir_entry *src = this->findDirectoryEntry(filepath1);
    dir_entry *dest = this->findDirectoryEntry(filepath2);
    if (src == nullptr || dest == nullptr)
        return FILE_NOT_FOUND;

    int srcBlock = src->first_blk;
    int destBlock = dest->first_blk;
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
        uint8_t buffer[BLOCK_SIZE];
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
            contentSrc = contentSrc.substr(contentSrc.find("\n"));
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
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    int currentDirectoryBlock = this->currentWorkingDir.first_blk;
    int newDirBlock = this->getFreeBlock();
    dir_entry newDirEntry((char *)dirpath.c_str(), 0, newDirBlock, TYPE_DIR, READ | WRITE | EXECUTE);
    this->writeDirectoryEntry(newDirEntry);
    

    std::string parrent = "..";
    int parentBlock = this->currentWorkingDir.first_blk;
    dir_entry parentDirEntry((char*)parrent.c_str(), 0, parentBlock, TYPE_DIR, READ | WRITE | EXECUTE);
    dir_entry dirBefore = this->currentWorkingDir;
    this->currentWorkingDir = newDirEntry;

    int previousDirCount = this->dirCount;
    this->dirCount = 0;
    this->writeDirectoryEntry(newDirEntry, true);
    this->writeDirectoryEntry(parentDirEntry, true);
    this->currentWorkingDir = dirBefore;
    this->dirCount = previousDirCount;

    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath, bool muteCall)
{   
    if(!muteCall)
        std::cout << "FS::cd(" << dirpath << ")\n";
    dir_entry* entry = this->findDirectoryEntry(dirpath); 
    
    if(dirpath.compare("..") == 0){
        
       this->currentWorkingDir = this->dirEntries[DIR_PARENT];
       this->loadNewDirectory();
       this->currentWorkingDir = this->dirEntries[DIR_SELF]; 
    }
    
    if (entry == nullptr)
    {
        std::cout << "Directory not found!\n";
        return FILE_NOT_FOUND;
    }
    else if(dirpath.compare("..") != 0){
        this->currentWorkingDir = *entry; 
        this->loadNewDirectory();
    }
    
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd()
{
    std::cout << "FS::pwd()\n";

    dir_entry* parent = this->findDirectoryEntry("..");
    if(parent == nullptr)
    {
        std::cout << "/" << "\n";
        return 0;
    }

    std::string path = currentWorkingDir.file_name;
    
    std::string currentFilename = "";
    int currentBlock = parent->first_blk;
    while(currentFilename.compare("/") != 0){
        uint8_t buffer[BLOCK_SIZE];
        this->disk.read(currentBlock, buffer);
        std::string serializedDirEntries = "";

        serializedDirEntries.assign((char*)buffer, BLOCK_SIZE);

        currentFilename = serializedDirEntries.substr(0, FILENAME_CHARS);
        currentFilename = this->dirParseAttr(currentFilename);

        if(currentFilename.compare("/") != 0)
            path = currentFilename +  "/" + path;
        else
            path = currentFilename + path;
        
        serializedDirEntries = serializedDirEntries.substr(DIR_ENTRY_SIZE + FILENAME_CHARS + SIZE_CHARS); 
        currentBlock = atoi(this->dirParseAttr(serializedDirEntries.substr(0, FIRST_BLK_CHARS)).c_str());
    }
    std::cout << path << "\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}

// ISSUE1
// Might be a potentiall problem becuase c strings are null terminated!!!!!!
dir_entry::dir_entry(char *file_name, uint32_t size, uint16_t first_blk, uint8_t type, uint8_t access_rights)
{
    memset(this->file_name, 0, sizeof(this->file_name));
    strncpy(
        this->file_name,
        file_name,
        sizeof(this->file_name) - 1);

    std::cout << "CREATING ENTRY WITH NAME: " << this->file_name << "\n";
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

void dir_entry::safeFileName(std::string filename)
{
    memset(this->file_name, 0, sizeof(this->file_name));
    strncpy(
        this->file_name,
        filename.c_str(),
        sizeof(this->file_name) - 1);
}

// FORMAT name-size-
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
