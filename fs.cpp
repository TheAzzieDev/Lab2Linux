#include <cstdint>
#include <cstdio>
#include <iostream>
#include <fstream>
#include "fs.h"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
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
    return 0;
}

int
FS::getFreeBlock(int blockBefore)
{
    for(int i = FAT_BLOCK + 1; i < NUMBER_OF_BLOCKS; i++){
        if(this->fat[i] == FAT_FREE){
            this->fat[i] = FAT_EOF;
            if(blockBefore != -1)
                this->fat[blockBefore] = i;
            return i;
        }
    }
    return -1;
}

bool
FS::fileExists(std::string filename)
{
    int block = 0;
    uint8_t* buffer;
    disk.read(block, buffer);

    return false;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";
    std::cout << "FS::create(" << filepath << ")\n";
    std::ifstream f(filepath.c_str());
    bool fileExists = f.good();
    std::cout << "Ran this many times \n";
    if(fileExists) 
        return 304;
    f.close();

    std::ofstream file(filepath.c_str(), std::ios::out);
    std::string userInput = "NONE";
    std::cout << "Ran this many times \n";

    int totalSize = 0;
    int lineCount = 0;
    int block = this->getFreeBlock();

    while(std::getline(std::cin, userInput) && userInput.size()){
        totalSize += userInput.size();
        if(lineCount != 0)
            userInput = "\n" + userInput;
        if(totalSize > block){
            block = this->getFreeBlock();
        }
        disk.write(block, (uint8_t*)userInput.c_str());
        lineCount++;
    }




    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::cout << "FS::ls()\n";
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
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
