#pragma warning(disable : 4996)
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include "fs.h"

std::string FS::DirParseAttr(std::string inputString) 
{

    while (inputString.size() > 0) {
        std::string delimeter{ inputString[0] };
        if (delimeter.compare(PLACE_HOLDER_CHAR) == 0) {
            inputString = inputString.substr(1);
        }
        else
            return inputString; 
    }
    return ""; 
}

FS::FS(){
    std::cout << "FS::FS()... Creating file system\n";
    this->loadDirectory();
    this->dirCount = 0;
}

FS::~FS()
{

}

// formats the disk, i.e., creates an empty file system
//FORMAT WORKS!!!
int
FS::format()
{
    std::cout << "FS::format()\n";
    this->fat[ROOT_BLOCK] = ROOT_BLOCK;
    this->fat[FAT_BLOCK] = FAT_BLOCK;
    for(int i = FAT_BLOCK + 1; i < NUMBER_OF_BLOCKS; i++){
        this->fat[i] = FAT_FREE;
    }
    for (int i = 0; i < NUMBER_OF_BLOCKS; i++) {
        std::string noData = "";
        this->disk.write(i, (uint8_t*)noData.c_str());  
    }
    return 0;
}

int
FS::getFreeBlock(int blockBefore)
{
    for(int i = FAT_BLOCK + 1; i < NUMBER_OF_BLOCKS; i++){
        if(this->fat[i] == FAT_FREE){
            this->fat[i] = FAT_EOF;
            if (blockBefore != FAT_EOF) { 
                this->fat[blockBefore] = i;
            }
            return i;
        }
    }
    return -1;
}

int FS::writeDirectoryEntry(dir_entry entry) 
{
    std::string serializedEntry = entry.serializeEntry(); 
    std::string previousData = "";
    uint8_t buffer[BLOCK_SIZE]; 

    this->disk.read(ROOT_BLOCK, buffer); 
    previousData = (char*)buffer;
    previousData += serializedEntry; 
    if (previousData.size() > BLOCK_SIZE) {
        return BLOCK_FULL;
    }
    this->disk.write(ROOT_BLOCK, (uint8_t*)previousData.c_str());
    this->dirEntries[this->dirCount++] = entry; 
    return 0;
}

dir_entry* FS::findDirectoryEntry(std::string filepath)
{
    bool found = false;
    int index = 0;
    while (!found && index < DIR_ENTRY_SIZE) { 
        dir_entry& entry = this->dirEntries[index++]; // Creates a reference
        std::string name = entry.file_name;  
        if (filepath.compare(name) == 0) {
            return &entry;  // if not for the first step we return reference to an object which no longer exists
        }
    }
    return nullptr;
}



void FS::loadDirectory()
{
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(ROOT_BLOCK, buffer);
    std::string bufferResult = (char*)buffer;

    while (bufferResult.size() != 0) { 
        std::string currentEntry = bufferResult.substr(0, DIR_ENTRY_SIZE);
        std::string filename = this->DirParseAttr(currentEntry.substr(0, FILENAME_CHARS));
        uint32_t fileSize = atoi(this->DirParseAttr(currentEntry.substr(FILENAME_CHARS, SIZE_CHARS)).c_str()); 
        uint16_t first_blk = atoi(this->DirParseAttr(currentEntry.substr(SIZE_CHARS + FILENAME_CHARS, FIRST_BLK_CHARS)).c_str()); 
        uint8_t type = atoi(this->DirParseAttr(currentEntry.substr(FIRST_BLK_CHARS + SIZE_CHARS + FILENAME_CHARS, TYPE_CHARS).c_str()).c_str());
        uint8_t access_rights = atoi(this->DirParseAttr(currentEntry.substr(TYPE_CHARS + FIRST_BLK_CHARS + SIZE_CHARS + FILENAME_CHARS, TYPE_CHARS)).c_str()); 

        dir_entry newEntry((char*)filename.c_str(), fileSize, first_blk, type, access_rights);
        this->dirEntries[this->dirCount++] = newEntry;

        bufferResult = bufferResult.substr(DIR_ENTRY_SIZE);
    }
}

void FS::copyToDirEntries(dir_entry& entry, dir_entry other)   
{
    for (int i = 0; i < FILENAME_CHARS; i++) {
        entry.file_name[i] = other.file_name[i];
    }
    entry.size = other.size;
    entry.first_blk = other.first_blk;
    entry.type = other.type;
    entry.access_rights = other.access_rights;
}

int
FS::getBlock(std::string filename)
{
    for (int block = FAT_BLOCK + 1; block < NUMBER_OF_BLOCKS; block++) { 
        uint8_t buffer[BLOCK_SIZE];
        disk.read(block, buffer);
        std::string myString = (char*)buffer;
        std::string filenameInDisk = myString.substr(0, myString.find("\n"));
        if (filename.compare(filenameInDisk) == 0) {
            return block; 
        }
    }
  
    return 0; 
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
//CHANGE LATER
int
FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";

    if (this->getBlock(filepath))
        return FILE_EXISTS;  

    std::string userInput = "NONE";

    int totalSize = 0;
    int lineCount = 0;
    int position = 0;
    int block = this->getFreeBlock();
    int firstBlock = block; 

    uint8_t buffer[BLOCK_SIZE] = {};

    std::string filenameInDisk = "";
    int amountOfPadding = FILENAME_CHARS - filepath.size();
    for (int i = 0; i < amountOfPadding; i++) {  
        filenameInDisk += PLACE_HOLDER_CHAR;
    }
    filenameInDisk += filepath;  


    strcpy((char *)buffer, (filenameInDisk + "\n").c_str()); 
    position += (filenameInDisk + "\n").size();  
    totalSize += position;

    while(std::getline(std::cin, userInput) && userInput.size()){
        if(lineCount != 0)
            userInput = "\n" + userInput;
        totalSize += userInput.size(); 
        if(totalSize > BLOCK_SIZE){
            block = this->getFreeBlock(block); 
        }
        int userInputIndex = 0; 
        for (int i = position; i < totalSize; i++) { 
            buffer[i] = userInput[userInputIndex]; 
            userInputIndex++;
        }
        lineCount++;
        position = totalSize; 
    }
   
    disk.write(block, (uint8_t*)buffer);   
    dir_entry newEntry((char*)filepath.c_str(), totalSize, firstBlock, 0, std::ios::in | std::ios::out);
    this->writeDirectoryEntry(newEntry);
    

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    int index = 0;
    while (index < DIR_ENTRY_SIZE) {
        dir_entry entry = this->dirEntries[index++];
        std::string filename = entry.file_name; 
        if (filename.compare(filepath) == 0) {
            int block = entry.first_blk;
            while (block != FAT_EOF) {
                uint8_t buffer[BLOCK_SIZE];
                this->disk.read(block, buffer);
                std::string content = (char*)buffer;
                 
                std::cout << content.substr(content.find("\n") + 1) << "\n";
                block = this->fat[block];
            }
            return 0;
        }
    }
   
    return FILE_NOT_FOUND;  
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::cout << "FS::ls()\n"; 
    int index = 0;
    while (index < this->dirCount) { 
        dir_entry currentEntry = this->dirEntries[index++]; 
        std::string filename = currentEntry.file_name;  
        if(filename.compare("") != 0)
            std::cout << filename << "\n";
    }

    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// DONT 
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    dir_entry* source = this->findDirectoryEntry(sourcepath);
    dir_entry* dest = this->findDirectoryEntry(destpath);
    if (source == nullptr)
        return FILE_NOT_FOUND;
    if(dest != nullptr)
        return FILE_EXISTS; 

    dir_entry sourceEntry = *source;
    dir_entry newEntry = sourceEntry;
    
    strcpy(newEntry.file_name, destpath.c_str());
    int newBlock = this->getFreeBlock();
    int sourceFatBlock = sourceEntry.first_blk; 
    newEntry.first_blk = newBlock;
    
    uint8_t buffer[BLOCK_SIZE];
    
    while (sourceFatBlock != FAT_EOF) { 
        this->disk.read(sourceFatBlock, buffer);
        std::string content = (char*)buffer; 
        content = content.substr(content.find("\n") + 1);
        this->disk.write(newBlock, (uint8_t*)content.c_str()); 
        sourceFatBlock = this->fat[sourceFatBlock];
        if(sourceFatBlock != FAT_EOF)  
            newBlock = this->getFreeBlock(newBlock); 
    }

    this->writeDirectoryEntry(newEntry);
    return 0; 
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    dir_entry* destinationEntryPtr = this->findDirectoryEntry(destpath);
    dir_entry* sourceEntryPtr = this->findDirectoryEntry(sourcepath); 
    if (destinationEntryPtr != nullptr) { 
        return FILE_EXISTS; 
    }
    if (sourceEntryPtr == nullptr){
        return FILE_NOT_FOUND; 
    }
    
    dir_entry& sourceEntry = *sourceEntryPtr; 

    uint8_t buffer[BLOCK_SIZE]; 
    this->disk.read(sourceEntry.first_blk, buffer);
    std::string firstBlockContent = (char*)buffer;

    firstBlockContent = firstBlockContent.substr(firstBlockContent.find("\n"));
    
    std::string filenameInDisk = ""; 
    int amountOfPadding = FILENAME_CHARS - destpath.size();   
    for (int i = 0; i < amountOfPadding; i++) {  
        filenameInDisk += PLACE_HOLDER_CHAR; 
    }
    filenameInDisk += destpath;

    std::string newFirstBlockContent = filenameInDisk + firstBlockContent; 
    this->disk.write(sourceEntry.first_blk, (uint8_t*)newFirstBlockContent.c_str());   
    
    strcpy(sourceEntry.file_name, destpath.c_str()); 
    

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    std::cout << "FS::pwd()\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}

//ISSUE1
//Might be a potentiall problem becuase c strings are null terminated!!!!!!
dir_entry::dir_entry(char* file_name, uint32_t size, uint16_t first_blk, uint8_t type, uint8_t access_rights)
{
    std::strcpy(this->file_name, file_name);
    this->size = size;
    this->first_blk = first_blk;
    this->type = type;
    this->access_rights = access_rights;
}

dir_entry::dir_entry() 
{
    std::strcpy(this->file_name, ""); 
    this->size = 0;
    this->first_blk = 0; 
    this->type = 0; 
    this->access_rights = std::ios::in | std::ios::out; 
}

dir_entry& dir_entry::operator=(const dir_entry& other)
{
    // TODO: insert return statement here 
    for (int i = 0; i < FILENAME_CHARS; i++) {
        this->file_name[i] = other.file_name[i];
    }
    this->size = other.size;
    this->first_blk = other.first_blk;
    this->type = other.type;
    this->access_rights = other.access_rights;
    return *this;
}


//FORMAT name-size-
std::string dir_entry::serializeEntry()
{
    std::string toReturn = "";
    std::string filenameString = (char*)this->file_name;
    std::string placeHolder = PLACE_HOLDER_CHAR; 
    int charsCap = (sizeof(this->file_name) / sizeof(char));  

    for (int i = 0; i < charsCap - filenameString.size(); i++)     
        toReturn += placeHolder; 
    toReturn += this->file_name;   

    std::string sizeString = std::to_string(this->size); 
    int sizeOfSizeString = sizeString.size();

    for (int i = 0; i < sizeof(this->size) - sizeOfSizeString; i++)
        toReturn += placeHolder; 
    toReturn += sizeString; 

    std::string blkString = std::to_string(this->first_blk);  
    int sizeOfBlkString = blkString.size();   

    for (int i = 0; i < sizeof(this->first_blk) - sizeOfBlkString; i++) 
        toReturn += placeHolder;   
    toReturn += blkString; 
    toReturn += std::to_string(this->type) + std::to_string(this->access_rights);

    return toReturn;  
}
