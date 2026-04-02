/******************************************************************************
 https://www.youtube.com/watch?v=U1I5UY_vWXI 
 THIS A FILE WHICH IS MYSELF USE FOR TESTING
 *****************************************************************************/

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
#include <fcntl.h>
#include "MyTest.h"
#include "fs.h"

#define PRINTDIV std::cout <<  "================================================================================" << std::endl
#define PRINTDIV2 std::cout << "----------------------------------------" << std::endl

std::string commands_str[] = {
    "format", "create", "cat", "ls",
    "cp", "mv", "rm", "append",
    "mkdir", "cd", "pwd",
    "chmod",
    "help", "quit"
};

Shell::Shell()
{
    std::cout << "Creating and starting shell...\n";
}

Shell::~Shell()
{
    std::cout << "Exiting shell...\n";
}


//REMEMBER I HAVE TAMPERED WITH THIS FILE a little bit
void
Shell::run()
{
    std::string cmd, arg1, arg2;
    int ret_val = 0;
    int fd[2];
    int fw;
    std::string input1 = "hej heja hejare\n";
    std::string input2 = "hej heja hejare hejast\n";

    PRINTDIV;
    std::cout << "\\ / \\ / \\ / \\ / \\ / \\ / \\     new test session     / \\ / \\ / \\ / \\ / \\ / \\ / \\ /" << std::endl;
    PRINTDIV;
    std::cout << "Starting test sequence..." << std::endl;
    PRINTDIV;
    std::cout << "Task 1 ..." << std::endl;
    PRINTDIV2;

    std::cout << "Testing format()..." << std::endl;
    ret_val = filesystem.format();

    bool testVar = true;
    if(filesystem.fat[0] != 0 || filesystem.fat[1] != 1)
        testVar = false;
    for(int i = FAT_BLOCK + 1; i < NUMBER_OF_BLOCKS; i++){
        if(filesystem.fat[i] != 0) 
            testVar = false;
    }

    if (ret_val || !testVar) {
        std::cout << "Error: format failed, error code " << ret_val << std::endl;
        return;
    }
    else
        std::cout << "SUCCESS: FORMAT IS WORKING!!! " << std::endl;


    std::cout << "Running create! " << std::endl;
    //filesystem.cat("test2.txt");
    std::cout << "\n";


    std::string name = "";  
    for (int i = 0; i < 55; i++) {
        name += "A";
    }
    name += "\0";
    
   
    dir_entry dirEntryTest((char*)name.c_str(), 4000, 2, 0, std::ios::in | std::ios::out);
    std::string serializedString = dirEntryTest.serializeEntry();
    filesystem.writeDirectoryEntry(dirEntryTest); 
    dir_entry entryMan; 
    std::string enFileName = entryMan.file_name; 
    std::cout << "Entry Looks Like: " << (enFileName.compare("")) << "\n";
  

    return;

    // check that the disk is empty

}
