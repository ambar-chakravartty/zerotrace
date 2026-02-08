#include "include/cert.hpp"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <array>
#include <iomanip>
#include <sstream>

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

std::string toHex(const std::array<uint8_t, 32>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

nlohmann::json makeChainRequest(
    const std::array<uint8_t,32>& certHash,
    const std::array<uint8_t,32>& devHash,
    uint8_t wipeMethod
) {
    return {
        {"cert_hash", "0x" + toHex(certHash)},
        {"device_hash", "0x" + toHex(devHash)},
        {"wipe_method", wipeMethod}
    };
}


bool recordWipeViaHelper(const nlohmann::json& payload) {
    CURL* curl = curl_easy_init();
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL,
        "http://127.0.0.1:8080/record-wipe");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
        payload.dump().c_str());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* ptr, size_t size, size_t nmemb, void* userdata) {
            auto* s = static_cast<std::string*>(userdata);
            s->append(ptr, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    auto res = nlohmann::json::parse(response);
    return res["status"] == "ok";
}
