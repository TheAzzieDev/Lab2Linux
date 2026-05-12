#include <iostream>
#include <cstdint>
#include <tuple>
#include <memory>
#include <vector>
#include "disk.h"

#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0
#define FAT_BLOCK 1
#define FAT_FREE 0
#define FAT_EOF -1
#define NUMBER_OF_BLOCKS 2048
#define FILE_EXISTS 304
#define FILE_NOT_FOUND 404
#define BLOCK_FULL 413 
#define FILENAME_TOO_LARGE 413
#define NO_VALID_INDEX -1 
#define ENTRY_IS_DIR 415
#define ENTRY_IS_FILE 414
#define ENTRY_CANNOT_BE_DELETED 405
#define DIR_SELF 0
#define DIR_PARENT 0
#define DIR_ENTRY_SIZE 64
#define AMOUNT_OF_DIRS 64
#define PLACE_HOLDER_CHAR "?"
#define FILENAME_CHARS 56
#define SIZE_CHARS 4
#define FIRST_BLK_CHARS 2
#define TYPE_CHARS 1
#define ACCESS_RIGHTS_CHARS 1
#define WRONG_PATH_FORMAT 406
#define WRONG_PERMISSION_FORMAT 406
#define WRONG_PERMISSIONS 406
#define DIRECTORY_RESTRICTS_ACCESS 403
#define STANDARD_DIR_COUNT 2



#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01

struct dir_entry {
    char file_name[56]; // name of the file / sub-directory
    uint32_t size; // size of the file in bytes
    uint16_t first_blk; // index in the FAT for the first block of the file

    uint8_t type; // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)

    dir_entry(char* file_name, uint32_t size, 
    uint16_t first_blk,
    uint8_t type,
    uint8_t access_rights);

    dir_entry(); 
    dir_entry& operator=(const dir_entry& other); 



    std::string serializeEntry(); 

};


/****************************
 * ----PERMISSIONS TABLE----
 * 1 = --x
 * 2 = -w-
 * 3 = -wx
 * 4 = r-
 * 5 = r-x
 * 6 = rw-
 * 7 = rwx
 ****************************/

/******************
 * DOCUMENTATION HAS BEEN MOVED TO fs.cpp file
 * TIP: Hover over functions to see their usage
******************/

 
class FS {
private:
    int dirCount; 
    int16_t fat[BLOCK_SIZE/2]; 
    Disk disk;     
    dir_entry dirEntries[BLOCK_SIZE / sizeof(dir_entry)]{};    
    dir_entry currentWorkingDir;  

    // HELPER FUNCTIONS

    void backtrack(std::vector<std::string> visitedDirectories);

    int cdHelper(std::string dirpath);

    int writeDirectoryEntry(dir_entry entry, bool dontAddEntry = false); 

    int getFreeDirEntryIndex();
    int getDirIndex(std::string path);

    void loadDirectory(); 
    void loadNewDirectory();
    void deserializeEntries(int block);
    
    void safeString(std::string& str);

    std::unique_ptr<std::tuple<std::string, int, std::string>> parsePath(std::string path, bool fromChmod = false);
    std::string addPadding(std::string filepath); 
    std::unique_ptr<dir_entry> findDirectoryEntry(std::string filepath); 
    std::string dirParseAttr(std::string inputString);   

    int getBlock(std::string filename);
    int getFreeBlock(int blockBefore = -1);

    bool hasWritePerm(dir_entry entry, bool muteCall = false);
    bool hasExecutePerm(dir_entry entry, bool muteCall = false);
    bool hasReadPerm(dir_entry entry, bool muteCall = false);

public:

    FS();
    ~FS();
   
    int format();

    int create(std::string filepath);

    int cat(std::string filepath);
  
    int ls();

    int cp(std::string sourcepath, std::string destpath);

    int mv(std::string sourcepath, std::string destpath);

    int rm(std::string filepath);

    int append(std::string filepath1, std::string filepath2);

    int mkdir(std::string dirpath);

    int cd(std::string dirpath, bool muteCall = false);

    int pwd();

    int chmod(std::string accessrights, std::string filepath);
};

#endif
