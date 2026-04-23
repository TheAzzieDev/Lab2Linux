#include <iostream>
#include "fs.h"



class Shell {
private:
    FS filesystem;
public:
    Shell();
    ~Shell();
    void run();
};


