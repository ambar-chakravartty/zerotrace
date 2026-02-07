#include "include/dev.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

// Helper to read a single line from a file
static std::string readFileLine(const fs::path& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::string value;
        std::getline(file, value);
        // Trim whitespace/newlines
        value.erase(value.find_last_not_of(" \n\r\t") + 1);
        return value;
    }
    return "";
}

std::vector<Device> getDevices() {
    std::vector<Device> devices;
    std::string sysBlockPath = "/sys/block";

    if (!fs::exists(sysBlockPath)) {
        std::cerr << "Error: /sys/block does not exist." << std::endl;
        return devices;
    }

    for (const auto& entry : fs::directory_iterator(sysBlockPath)) {
        if (!entry.is_directory()) continue;

        std::string deviceName = entry.path().filename().string();
        
        // Filter out loop, ram, and other non-physical devices
        if (deviceName.rfind("loop", 0) == 0 || 
            deviceName.rfind("ram", 0) == 0 ||
            deviceName.rfind("dm-", 0) == 0 ||
            deviceName.rfind("sr", 0) == 0) { // sr is usually CD-ROM
            continue;
        }

        Device dev;
        dev.name = deviceName;
        dev.path = "/dev/" + deviceName;
        
        // Read size (in sectors, usually 512 bytes)
        std::string sizeStr = readFileLine(entry.path() / "size");
        try {
            // Check if size is empty or not a number
            if (!sizeStr.empty()) {
                dev.sizeBytes = std::stoull(sizeStr) * 512;
            } else {
                dev.sizeBytes = 0;
            }
        } catch (...) {
            dev.sizeBytes = 0;
        }

        dev.isRemovable = (readFileLine(entry.path() / "removable") == "1");
        dev.isReadOnly = (readFileLine(entry.path() / "ro") == "1");
        
        // Try to get model
        // Path varies: /sys/block/sda/device/model or /sys/block/nvme0n1/device/model
        dev.model = readFileLine(entry.path() / "device" / "model");
        if (dev.model.empty()) {
             // Try a deeper search or alternate path if needed, 
             // but 'device/model' covers most ATA/SCSI.
             // NVMe often puts model in /sys/class/nvme/nvmeX/model or similar, 
             // but /sys/block/nvme0n1/device/model acts as a symlink or direct path often.
             // Let's check the specific NVMe path from my previous 'ls' output:
             // It showed /sys/block/nvme0n1/device/model
             // So this should work.
        }

        // Determine Type
        if (deviceName.rfind("nvme", 0) == 0) {
            dev.type = "NVMe";
        } else if (deviceName.rfind("sd", 0) == 0) {
            // Could be SATA, SAS, or USB.
            // Check if usb exists in the device path
            // realpath /sys/block/sdX/device
            // This is a bit complex to do perfectly portable, simple heuristic:
            dev.type = "ATA/SCSI"; // Broad category
        } else if (deviceName.rfind("mmcblk", 0) == 0) {
            dev.type = "SD/MMC";
        } else {
            dev.type = "Unknown";
        }

        // Determine Supported Wipe Methods
        // All writable block devices support overwrite
        if (!dev.isReadOnly) {
            dev.supportedWipeMethods.push_back(WipeMethod::PLAIN_OVERWRITE);
            dev.supportedWipeMethods.push_back(WipeMethod::ENCRYPTED_OVERWRITE);
            
            // Heuristic for Firmware Erase
            if (dev.type == "NVMe") {
                // Assume NVMe supports sanitize commands
                dev.supportedWipeMethods.push_back(WipeMethod::FIRMWARE_ERASE);
            } else if (dev.type == "ATA/SCSI") {
                // Assume ATA supports Secure Erase (though it might be frozen)
                dev.supportedWipeMethods.push_back(WipeMethod::ATA_SECURE_ERASE);
            }
        }

        devices.push_back(dev);
    }
    return devices;
}




