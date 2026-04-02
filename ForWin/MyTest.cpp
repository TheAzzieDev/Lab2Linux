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

    dir_entry entryT;
    dir_entry entryA;
    entryT = entryA; 

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
    filesystem.create("MARIA.txt");
    filesystem.create("test3.txt");

    uint8_t buffer[BLOCK_SIZE];
    filesystem.disk.read(3, buffer);
    std::string result = (char*)buffer;
    std::cout << "result: " << result << "\n";
    std::cout << "\n";
    filesystem.cat("test3.txt");
    filesystem.ls(); 
    std::cout << "\n";




    return;

    // check that the disk is empty

}
