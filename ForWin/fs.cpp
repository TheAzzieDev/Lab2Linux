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
    this->format(); 
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
        uint8_t empty[BLOCK_SIZE] = { 0 };  
        this->disk.write(i, empty);    
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

void FS::testArgs(uint8_t* buffer)
{
    std::string test = (char*)buffer;
    std::cout << test << "\n";

    uint8_t anotherTest[BLOCK_SIZE]; 
    for (int i = 0; i < test.size(); i++) {
        anotherTest[i] = test[i]; 
    }
    anotherTest[test.size()] = '\0';
    std::string sometingElse = (char*)anotherTest; 
    std::cout << sometingElse << "\n";   
}

int FS::writeDirectoryEntry(dir_entry entry) 
{

    int index = this->getFreeDirEntryIndex(); 
    if (index == NO_VALID_INDEX) 
        return NO_VALID_INDEX;  
    std::string serializedEntry = entry.serializeEntry(); 
    std::string previousData = "";
    uint8_t buffer[BLOCK_SIZE]; 

    this->disk.read(ROOT_BLOCK, buffer); 
    int amountToRead = this->dirCount * DIR_ENTRY_SIZE;
    previousData.assign((char*)buffer, amountToRead);     
    previousData += serializedEntry; 
    int sizePrev = previousData.size(); 
    
    this->safeString(previousData);  

    this->disk.write(ROOT_BLOCK, (uint8_t*)previousData.c_str());
    this->dirEntries[index] = entry;  
    return 0;
}

dir_entry* FS::findDirectoryEntry(std::string filepath)
{
    bool found = false;
    int index = 0;
    while (!found && index < this->dirCount) {  
        dir_entry& entry = this->dirEntries[index++]; // Creates a reference
        std::string name = entry.file_name;  
        if (filepath.compare(name) == 0) {
            return &entry;  // if not for the first step we return reference to an object which no longer exists
        }
    }
    return nullptr;
}

int FS::getFreeDirEntryIndex()
{

    int numberOfDirEntries = AMOUNT_OF_DIRS;  
    if (this->dirCount != numberOfDirEntries) {
        for (int i = 0; i < numberOfDirEntries; i++) { 
            std::string filename = "";  
            filename = this->dirEntries[i].file_name;   
            if (filename.compare("") == 0) { 
                this->dirCount++;
                return i;
            }
        }
    }
    return NO_VALID_INDEX;  
}



void FS::loadDirectory()
{
    uint8_t buffer[BLOCK_SIZE];
    this->disk.read(ROOT_BLOCK, buffer);
    if (buffer[0] == '\0')
        return;
    std::string bufferResult = "";
    bufferResult = (char*)buffer; 

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

void FS::printer(int block) {
    if (block == FAT_EOF){
        std::cout << "END OF FILE!\n";
        return;
    }
    uint8_t newBuffer[BLOCK_SIZE];  

    while (block != FAT_EOF && block != FAT_FREE) {  
        std::cout << "___________________________\n";
        this->disk.read(block, newBuffer);
        std::string result = "";
        result.assign((char*)newBuffer, BLOCK_SIZE);
        std::cout << result << "\n";
        std::cout << "___________________________\n";
        block = this->fat[block];
    }
}

void FS::safeString(std::string& str)
{
    int toPadd = BLOCK_SIZE - str.size(); 
    for (int i = 0; i < toPadd; i++)
        str += '\0'; 
}

std::string FS::addPadding(std::string filepath) 
{
    std::string filenameInDisk = "";
    int amountOfPadding = FILENAME_CHARS - filepath.size();
    for (int i = 0; i < amountOfPadding; i++) {
        filenameInDisk += PLACE_HOLDER_CHAR;
    }
    filenameInDisk += filepath;
    return filenameInDisk;
}

int
FS::getBlock(std::string filename)
{
    for (int block = FAT_BLOCK + 1; block < NUMBER_OF_BLOCKS; block++) { 
        uint8_t buffer[BLOCK_SIZE];
        disk.read(block, buffer);
        std::string myString = "";
        myString.assign((char*)buffer, BLOCK_SIZE);
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
//We are not checking length of filenames.
int
FS::create(std::string filepath)
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
    //for (int i = 0; i < BLOCK_SIZE; i++) {   
    //    toAddString += "A";
    //}
    //currentSize = toAddString.size();  
    int totalSizeOfFile = 0;  
    while (std::getline(std::cin, userInput) && userInput.size()) {
        currentSize += userInput.size();
        totalSizeOfFile += currentSize;  
        toAddString += userInput;
        while (currentSize > BLOCK_SIZE) { 
            std::string toWrite = toAddString.substr(0, BLOCK_SIZE);
            this->disk.write(block, (uint8_t*)toWrite.c_str());
            block = this->getFreeBlock(block); 
            toAddString = toAddString.substr(BLOCK_SIZE);
            currentSize = toAddString.size(); 
        }
    }

    this->safeString(toAddString);
    this->disk.write(block, (uint8_t*)toAddString.c_str());   

    dir_entry newEntry((char*)filepath.c_str(), totalSizeOfFile, firstBlock, 0, std::ios::in | std::ios::out); 
    this->writeDirectoryEntry(newEntry);
    
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    int index = 0;
    while (index < this->dirCount) { 
        dir_entry entry = this->dirEntries[index++];
        std::string filename = entry.file_name; 
        if (filename.compare(filepath) == 0) { 

            int block = entry.first_blk; 
            uint8_t buffer[BLOCK_SIZE]; 
            this->disk.read(block, buffer);  

            std::string content = "";
            content.assign((char*)buffer, BLOCK_SIZE);
            std::cout << content.substr(content.find("\n") + 1) << "\n";

            while (block != FAT_EOF) {
                block = this->fat[block];
                if (block != FAT_EOF) { 
                    this->disk.read(block, buffer);
                    content = ""; 
                    content.assign((char*)buffer, BLOCK_SIZE);   
                    std::cout << content; 
                }

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
// Fix
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
    if (destpath.size() > FILENAME_CHARS)
        return FILENAME_TOO_LARGE; 

    if (this->dirCount > AMOUNT_OF_DIRS)   
        return NO_VALID_INDEX; 

    dir_entry sourceEntry = *source;
    dir_entry newEntry = sourceEntry;
    
    strcpy(newEntry.file_name, destpath.c_str());
    int newBlock = this->getFreeBlock();
    int sourceFatBlock = sourceEntry.first_blk; 
    newEntry.first_blk = newBlock;
    
    uint8_t buffer[BLOCK_SIZE];


    std::string newContent = this->addPadding(destpath) + "\n";
    this->disk.read(sourceFatBlock, buffer);
  

    std::string content = "";
    content.assign((char*)buffer, BLOCK_SIZE);   
    newContent += content.substr(content.find("\n") + 1); 
    this->disk.write(newBlock, (uint8_t*)newContent.c_str()); 

    int previous = 0;
    int count = 0;
    sourceFatBlock = this->fat[sourceFatBlock]; 

    while (sourceFatBlock != FAT_EOF) {   
        newBlock = this->getFreeBlock(newBlock);  
        this->disk.read(sourceFatBlock, buffer); 
        newContent.assign((char*)buffer, BLOCK_SIZE);
        this->disk.write(newBlock, (uint8_t*)newContent.c_str()); 
        previous = sourceFatBlock; 
        sourceFatBlock = this->fat[sourceFatBlock];
        count++;
    } 
    if (count > 0) {
        this->disk.write(newBlock, (uint8_t*)newContent.c_str());  
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
    std::string firstBlockContent = "";
    firstBlockContent.assign((char*)buffer, BLOCK_SIZE); 

    firstBlockContent = firstBlockContent.substr(firstBlockContent.find("\n"));
    
    std::string filenameInDisk = this->addPadding(destpath);  

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
    dir_entry* entry = this->findDirectoryEntry(filepath);
    if (entry == nullptr)
        return FILE_NOT_FOUND;
    int block = entry->first_blk;  
    int previous = block; 
    while (block != FAT_EOF) {
        std::string emptyContent = "";
        this->safeString(emptyContent); 
        this->disk.write(block, (uint8_t*)emptyContent.c_str());   
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
// the end of file <filepath2>. The file <filepath1> is unchanged.


//ISSUE Possibly with writing blocks over range or something due to added \n character
int
FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    dir_entry* src = this->findDirectoryEntry(filepath1);  
    dir_entry* dest = this->findDirectoryEntry(filepath2);  
    if (src == nullptr || dest == nullptr)
        return FILE_NOT_FOUND;

    int srcBlock = src->first_blk;
    int destBlock = dest->first_blk;
    int destPrevBlock = destBlock;   

    //dest traverse till end of file
    while (destBlock != FAT_EOF) {
        destPrevBlock = destBlock; 
        destBlock = this->fat[destBlock];
    }
    destBlock = destPrevBlock;


    std::string toAdd = "";
    int count = 0;
    while (srcBlock != FAT_EOF) {
        std::string contentSrc = "";
        uint8_t buffer[BLOCK_SIZE];
        this->disk.read(srcBlock, buffer);  
        contentSrc.assign((char*)buffer, BLOCK_SIZE); 

        int indexOfNull = contentSrc.find('\0');
        if(indexOfNull != std::string::npos) 
            contentSrc = contentSrc.substr(0, indexOfNull);  
        contentSrc = toAdd + contentSrc; 

        if (count == 0)
        {
            this->disk.read(destBlock, buffer);
            toAdd += (char*)buffer;
            contentSrc = contentSrc.substr(contentSrc.find("\n"));
            toAdd += contentSrc;

            if (toAdd.size() > BLOCK_SIZE) {
                std::string toAddBlock = toAdd.substr(0, BLOCK_SIZE);
                this->disk.write(destBlock, (uint8_t*)toAddBlock.c_str());
                toAdd = toAdd.substr(BLOCK_SIZE);
            }
            else {
                this->safeString(toAdd);  
                this->disk.write(destBlock, (uint8_t*)toAdd.c_str());  
                toAdd = "";
            }
        }
           
        else {
            this->safeString(contentSrc);   
            destBlock = this->getFreeBlock(destBlock); 
            this->disk.write(destBlock, (uint8_t*)contentSrc.c_str()); 
       
        }

        srcBlock = this->fat[srcBlock];
        count++;
    }
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
