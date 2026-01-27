/**
 * @file      Logger.cpp
 * @brief     日志记录模块实现文件
 */

#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

// 定义静态数据成员
Logger *Logger::instance = nullptr;
std::mutex Logger::mutex;

// 静态成员初始化
Logger::Logger()
    : min_level(LogLevel::DEBUG), enable_console(true), enable_file(true),
      log_file_path("data/shopping_app.log") {
    // 尝试打开日志文件
    log_file.open(log_file_path, std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file: " << log_file_path << std::endl;
    }
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

Logger &Logger::get_instance() {
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr) {
        instance = new Logger();
    }
    return *instance;
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now(); // 获取时间点
    auto in_time_t =
        std::chrono::system_clock::to_time_t(now); // 转换为 time_t(C风格)

    std::stringstream ss;
    // 转换为 tm 结构体并格式化输出
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

void Logger::write_log(LogLevel level, const std::string &message,
                       const std::string &file, int line) {
    std::stringstream ss;

    // 格式: [时间] [级别] [文件:行号] 消息
    ss << "[" << get_timestamp() << "] "
       << "[" << level_to_string(level) << "] ";

    if (!file.empty() && line > 0) {
        // 只显示文件名，不显示完整路径
        size_t pos = file.find_last_of("/\\"); // 匹配最后一个“/” 或 "\"
        std::string filename =
            (pos != std::string::npos) ? file.substr(pos + 1) : file;
        ss << "[" << filename << ":" << line << "] ";
    }

    ss << message << std::endl;

    std::string log_entry = ss.str();

    // 输出到控制台
    if (enable_console) {
        if (level >= LogLevel::ERROR) {
            std::cerr << log_entry;
        } else {
            std::cout << log_entry;
        }
    }

    // 输出到文件
    if (enable_file && log_file.is_open()) {
        log_file << log_entry;
        log_file.flush(); // 确保立即写入
    }
}

void Logger::log(LogLevel level, const std::string &message,
                 const std::string &file, int line) {
    // 过滤低于最低级别的日志
    if (level < min_level) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    write_log(level, message, file, line);
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex);
    min_level = level;
}

void Logger::set_log_file(const std::string_view &path) {
    std::lock_guard<std::mutex> lock(mutex);

    // 关闭旧文件
    if (log_file.is_open()) {
        log_file.close();
    }

    // 设置新路径并打开
    log_file_path = path;
    log_file.open(log_file_path, std::ios::app);

    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file: " << log_file_path << std::endl;
    }
}

void Logger::set_console_output(bool enable) {
    std::lock_guard<std::mutex> lock(mutex);
    enable_console = enable;
}

void Logger::set_file_output(bool enable) {
    std::lock_guard<std::mutex> lock(mutex);
    enable_file = enable;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex);
    if (log_file.is_open()) {
        log_file.flush();
    }
}
