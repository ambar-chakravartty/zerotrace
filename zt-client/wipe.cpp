#include "include/wipe.hpp"
#include "include/dev.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <cstring>
#include <iostream>
#include <vector>


#define MP_NUM_PASSES 3

#include <sys/wait.h>
#include <vector>
#include <cstring>

static bool runHdparm(const std::vector<const char*>& args) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], const_cast<char* const*>(args.data()));
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool mpOverwrite(const std::string& devicePath){
    int fd = open(devicePath.c_str(), O_WRONLY | O_SYNC);
    if (fd < 0) {
        perror("open");
        return false;
    }

    uint64_t size = 0;
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        perror("BLKGETSIZE64");
        close(fd);
        return false;
    }

    constexpr size_t BLKSIZE = 1024 * 1024; // 1 MiB
    std::vector<char> zeroBuf(BLKSIZE, 0);


    for (int pass = 0; pass < MP_NUM_PASSES; pass++) {
        if (lseek(fd, 0, SEEK_SET) < 0) {
            perror("lseek");
            close(fd);
            return false;
        }

        uint64_t written = 0;
        while (written < size) {
            size_t toWrite = std::min<uint64_t>(BLKSIZE, size - written);
            ssize_t w = write(fd, zeroBuf.data(), toWrite);

            if (w < 0) {
                perror("write");
                close(fd);
                return false;
            }
            written += w;
        }

        if (fsync(fd) < 0) {
            perror("fsync");
            close(fd);
            return false;
        }
    }

    close(fd);
    return true;
    
    return true;
}


bool ataSecureErase(const std::string& devicePath) {
    const char* hdparm = "hdparm";
    const char* pass   = "wipe";

    /* Step 1: check security state */
    if (!runHdparm({hdparm, "-I", devicePath.c_str(), nullptr})) {
        std::cerr << "Failed to read drive identity\n";
        return false;
    }

    /* Step 2: set temporary password */
    if (!runHdparm({
            hdparm,
            "--user-master", "u",
            "--security-set-pass", pass,
            devicePath.c_str(),
            nullptr
        })) {
        std::cerr << "Failed to set ATA security password\n";
        return false;
    }

    /* Step 3: issue secure erase */
    std::cout << "Starting ATA Secure Erase on " << devicePath << "\n";

    if (!runHdparm({
            hdparm,
            "--user-master", "u",
            "--security-erase", pass,
            devicePath.c_str(),
            nullptr
        })) {
        std::cerr << "ATA Secure Erase failed\n";
        return false;
    }

    std::cout << "ATA Secure Erase completed successfully\n";
    return true;
}



bool wipeDisk(const std::string& devicePath, WipeMethod
        method){

    switch(method){
        case WipeMethod::ATA_SECURE_ERASE:
            return ataSecureErase(devicePath);
        case WipeMethod::FIRMWARE_ERASE:
            break;
        case WipeMethod::PLAIN_OVERWRITE:
            return mpOverwrite(devicePath);
        case WipeMethod::ENCRYPTED_OVERWRITE:
            break;
        default:
            std::cout << "Unsupported Wipe Method\n";

    }
    return false;
}

// Wipe a single partition
bool wipePartition(const std::string& partitionPath) {
    // Use the same overwrite logic as full disk
    return mpOverwrite(partitionPath);
}

