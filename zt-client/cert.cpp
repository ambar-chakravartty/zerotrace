#include "include/cert.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <array>

std::array<uint8_t, 32> sha256(const std::string& data) {
    std::array<uint8_t, 32> hash;
    SHA256(
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(),
        hash.data()
    );
    return hash;
}


std::array<uint8_t, 32> deviceIdentityHash(const WipeResult& r) {
    std::string id = r.device_model + "|" +
                     r.device_serial + "|" +
                     std::to_string(r.device_size);

    return sha256(id);
}

std::string generateCertificateJSON(const WipeResult& r) {
    nlohmann::json j;

    j["device_path"] = r.device_path;
    j["device_model"] = r.device_model;
    j["device_serial"] = r.device_serial;
    j["device_size"] = r.device_size;

    j["wipe_method"] = static_cast<int>(r.method);
    j["wipe_status"] = (r.status == WipeStatus::SUCCESS);

    j["start_time"] = r.start_time;
    j["end_time"] = r.end_time;
    j["tool_version"] = r.tool_version;

    return j.dump(); // no pretty-printing
}
