#pragma once
#include "httplib.h"
#include "json.hpp"
#include <optional>
#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT

using json = nlohmann::json;
using string = std::string;

struct AddressInfo {
    string country; // 国家
    string region;  // 省
    string city;    // 城市
    string isp;     // 互联网提供商？
};
