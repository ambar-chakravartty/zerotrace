#include <iostream>
#include "include/gui.hpp"

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h> // For geteuid
#endif

int main(int argc, char* argv[]) {
#if defined(__linux__) || defined(__APPLE__)
    // check for sudo permissions on Linux/Mac
    if(geteuid() != 0){
        std::cout << "Warning: This app usually needs superuser permissions to list block devices.\n"; 
        std::cout << "Running in GUI mode, but device list might be empty.\n";
    }
#endif

    runGui();

    return 0;
}
