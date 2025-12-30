#pragma once

#include <string>

namespace Utils {

// 获取数据库的文件路径
inline std::string get_database_path(const std::string &filename) {
    // DATA_PATH 是 CMake 传进来的宏，为指定路径
    return std::string(DATA_PATH) + "/" + filename;
}

} // namespace Utils
