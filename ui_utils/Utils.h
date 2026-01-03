#pragma once
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

// 这里放置一些单独使用的函数
namespace Utils {

// 格式化价格浮点数，避免多位小数点
inline std::string format_price(double price) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << price;
    return ss.str();
}

inline std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    // 格式化为：2025-12-19 12:00:01
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 格式化 time_t 为某秒的字符串
inline std::string time_to_string(const time_t time) {
    std::stringstream ss;
    // 格式化为：2025-12-19 12:00:01
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 格式化 time_t 为具体到某一天的字符串
inline std::string specific_hour_to_string(const time_t time) {

    std::stringstream ss;
    // 格式化为：2025-12-19
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d");
    return ss.str();
}

inline time_t add_days_to_time(const time_t base_time, const int days) {
    return base_time + days * 24 * 3600;
}

// 你以后还可以在这加别的工具，比如时间格式化等
// inline std::string get_current_time() { ... }
} // namespace Utils
