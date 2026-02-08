#ifndef CERT_HPP
#define CERT_HPP

#include <string>
#include <cstdint>
#include <array>
#include <nlohmann/json.hpp>
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

struct VerificationResult {
    bool verified;
    std::string errorMessage;
    uint64_t timestamp;
    uint8_t wipeMethod;
};

std::array<uint8_t, 32> sha256(const std::string& data);
std::array<uint8_t, 32> deviceIdentityHash(const WipeResult& r);
std::string generateCertificateJSON(const WipeResult& r);
nlohmann::json makeChainRequest(
    const std::array<uint8_t,32>& certHash,
    const std::array<uint8_t,32>& devHash,
    uint8_t wipeMethod
);
bool recordWipeViaHelper(const nlohmann::json& payload);
VerificationResult verifyCertificateFromFile(const std::string& filepath);
#endif
