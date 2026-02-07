#ifndef DEV_HPP
#define DEV_HPP

#include <string>
#include <vector>
#include <cstdint>

enum class WipeMethod {
    PLAIN_OVERWRITE,
    ENCRYPTED_OVERWRITE,
    FIRMWARE_ERASE,
    ATA_SECURE_ERASE
};


struct Device {
    std::string name;
    std::string path;
    uint64_t sizeBytes;
    bool isRemovable;
    bool isReadOnly;
    std::string model;
    std::string type; // "NVMe", "ATA", "USB", "Unknown"
    std::vector<WipeMethod> supportedWipeMethods;
};

std::vector<Device> getDevices();

void printDeviceList(const std::vector<Device>& devices);

#endif
