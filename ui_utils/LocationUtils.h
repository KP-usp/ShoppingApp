#pragma once

#include <httplib.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

using json = nlohmann::json;
using string = std::string;

struct AddressInfo {
    string country; // 国家
    string region;  // 省
    string city;    // 城市
    string isp;     // 互联网提供商？

    // 组合成字符串
    string to_string() {
        return country + " " + region + " " + city + " (" + isp + ")";
    }
};

class LocationUtils {
  public:
    static std::optional<AddressInfo> get_current_location() {
        try {

            // 创建 HTTP 客户端，连接定位 api 网站
            httplib::Client cli("http://ip-api.com");

            // 设置超时，防止网络卡死
            cli.set_connection_timeout(std::chrono::seconds(3));

            // 发送 GET 请求，以中文形式返w回
            auto res = cli.Get("/json/?lang=zh-CN");

            if (res && res->status == 200) {

                auto j = json::parse(res->body);
                if (j["status"] == "success") {
                    AddressInfo addr;
                    addr.country = j.value("country", "未知国家");
                    addr.region = j.value("regionName", "未知省份");
                    addr.city = j.value("city", "未知城市");
                    addr.isp = j.value("isp", "");
                    // 成功返回
                    return addr;
                }
            }
        } catch (const std::exception &e) {
            std::string path = "data/debug.log";

            std::ofstream outfile(path, std::ios_base::app);
            if (outfile.is_open()) {
                outfile << "Location API Error: " << e.what() << std::endl;
                outfile.close();
            }
        }
        return std::nullopt;
    }
};
