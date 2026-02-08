#ifndef WIPE_HPP
#define WIPE_HPP

#include <string>
#include "dev.hpp" 
#include "cert.hpp"

WipeResult wipeDisk(const std::string& devicePath, WipeMethod method);

#endif
