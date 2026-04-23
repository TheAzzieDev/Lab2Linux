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
    std::string hej = "\nHello There"; 
    filesystem.testArgs((uint8_t*)hej.c_str());  
  



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
    uint8_t buffer[BLOCK_SIZE];  

    std::cout << "Running create! " << std::endl;

    //WORKING FINE WITH MY TEST = ACCORDING WHAT I THINK IS RIGHT

    //CREATE IS FINE TO MY TESTS
    filesystem.create("MARIA.txt");
    filesystem.create("test3.txt"); 
    filesystem.printer(2); 

    //CAT IS WORKING WITH MY TESTS
    //filesystem.cat("MARIA.txt");
    std::cout << "\n";

    //LS IS WORKING WITH MY TESTS
    filesystem.ls();


    //CP IS WORKING WITH MY TESTS
    filesystem.cp("MARIA.txt", "heyThere.txt");

    //filesystem.cat("heyThere.txt");
    //filesystem.printer((filesystem.findDirectoryEntry("MARIA.txt"))->first_blk);  
    filesystem.printer((filesystem.findDirectoryEntry("heyThere.txt"))->first_blk);       

    //MV IS WORKING WITH MY TESTS
    filesystem.mv("MARIA.txt", "NewFile.txt");
    filesystem.printer(2); 
    filesystem.ls(); 


    //rm + MAX entries now implemented IS WORKING WITH MY TESTS
    filesystem.rm("NewFile.txt");
    filesystem.printer(2);
    filesystem.printer(3);
    filesystem.ls();

    filesystem.cat("test3.txt");


    //APPEND IS WORKING WITH MY TEST
    filesystem.create("append.txt");
    filesystem.append("test3.txt", "append.txt");
    filesystem.cat("append.txt");

    filesystem.printer((filesystem.findDirectoryEntry("append.txt"))->first_blk);  


   /* for (int i = 0; i < 64; i++) {
        int status = filesystem.create("testOf" + std::to_string(i) + ".txt");
        if (status == NO_VALID_INDEX) {
            std::cout << "NOVALIDINDEX";
        }
           
    }

    int status = filesystem.cp("testOf" + std::to_string(62) + ".txt", "new.txt");
    if (status == NO_VALID_INDEX) {
        std::cout << "NOVALIDINDEX________AGAIN";
    }*/



 /*  
    filesystem.disk.read(3, buffer);
    std::string result = (char*)buffer;
    std::cout << "result: " << result << "\n";
    std::cout << "\n";
    filesystem.cat("test3.txt");
    filesystem.cat("MARIA.txt");
    filesystem.ls();
    std::cout << "\n";

    filesystem.cat("test3.txt");
    filesystem.cat("MARIA.txt");

    filesystem.cp("MARIA.txt", "boober.txt"); 
    filesystem.mv("MARIA.txt", "cool.txt"); 
    filesystem.printer(2, buffer); 
    filesystem.printer(3, buffer);
    filesystem.printer(4, buffer);
    filesystem.cat("test3.txt");*/

    //create test
    //std::string userInput = "NONE";

    //int totalSize = 0;
    //int block = filesystem.getFreeBlock(); 
    //int firstBlock = block;

    //for (int i = 0; i < BLOCK_SIZE; i++) {
    //    buffer[i] = '\0';
    //}
    //
    //std::string filenameInDisk = filesystem.addPadding("test.txt") + "\n";


    //
    //totalSize += filenameInDisk.size();
    //std::string toAddString = "";

    //toAddString += filenameInDisk; 
    //for (int i = 0; i < BLOCK_SIZE - 9; i++) {
    //    toAddString += "A";
    //}
    //while (std::getline(std::cin, userInput) && userInput.size()) {
    //    totalSize += toAddString.size(); 
    //    toAddString += userInput;
    //    if (totalSize > BLOCK_SIZE) {
    //        std::string toWrite = toAddString.substr(0, BLOCK_SIZE);
    //        filesystem.disk.write(block, (uint8_t* )toWrite.c_str());     
    //        block = filesystem.getFreeBlock(block); 
    //        toAddString = toAddString.substr(BLOCK_SIZE); 
    //        totalSize = toAddString.size();  
    //    }
    //}

    //filesystem.safeString(toAddString);
    //filesystem.disk.write(block, (uint8_t*)toAddString.c_str());   
    //
    //filesystem.printer(5, buffer); 
    //filesystem.printer(6, buffer);  
    //result = (char*)buffer;

    //std::cout << result << "\n";

    //filesystem.cat("MARIA.txt");

    //filesystem.cp("MARIA.txt", "boober.txt");


    //filesystem.cat("boober.txt");
    //filesystem.ls();
    //filesystem.printer(4, buffer); 
    //filesystem.mv("MARIA.txt", "NILA.txt");
    //filesystem.printer(2, buffer); 

    //filesystem.printer(4, buffer); 

    return;

    // check that the disk is empty

}
