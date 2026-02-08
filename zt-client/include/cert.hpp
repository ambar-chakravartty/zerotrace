#ifndef CERT_HPP
#define CERT_HPP

#include <string>
#include <cstdint>
#include "dev.hpp"


enum class WipeStatus {
    SUCCESS,
    FAILURE
};

struct WipeResult {
    std::string device_path;
    std::string device_model;
    std::string device_serial;
    uint64_t    device_size;

    WipeMethod  method;
    WipeStatus  status;

    uint64_t start_time;
    uint64_t end_time;

    std::string tool_version;
};

#endif
