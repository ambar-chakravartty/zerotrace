#ifndef WIPE_HPP
#define WIPE_HPP

#include <string>
#include "dev.hpp"

// Wipes the entire disk using the specified method
bool wipeDisk(const std::string& devicePath, WipeMethod method);

// Wipes a specific partition
bool wipePartition(const std::string& partitionPath);

// ATA Secure Erase implementation
bool ataSecureErase(const std::string& devicePath);

// Multi-pass overwrite implementation
bool mpOverwrite(const std::string& devicePath);

#endif
