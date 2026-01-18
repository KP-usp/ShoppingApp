/**
 * @file      Logger.h
 * @brief     日志记录模块头文件
 * @details   定义了日志级别枚举(LogLevel)和日志记录类(Logger)，
 *            提供线程安全的日志记录功能，支持文件和控制台输出。
 */

#pragma once

#include <FileError.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>

/**
 * @brief 日志级别枚举
 *
 * 定义不同级别的日志，用于过滤重要程度不同的消息
 */
enum class LogLevel {
    DEBUG = 0,    // 调试信息
    INFO = 1,     // 一般信息
    WARNING = 2,  // 警告信息
    ERROR = 3,    // 错误信息
    CRITICAL = 4, // 严重错误
};

/**
 * @brief 日志记录类
 *
 * 单例模式的日志管理类，提供线程安全的日志记录功能。
 * 支持按级别过滤、文件输出、控制台输出等功能。
 */
class Logger {
  private:
    static Logger *instance; // 单例实例指针
    static std::mutex mutex; // 线程安全互斥锁

    std::ofstream log_file;    // 日志文件输出流
    LogLevel min_level;        // 最低记录级别
    bool enable_console;       // 是否启用控制台输出
    bool enable_file;          // 是否启用文件输出
    std::string log_file_path; // 日志文件路径

    /**
     * @brief 私有构造函数
     *
     * 单例模式，防止外部创建实例
     */
    Logger();

    /**
     * @brief 析构函数
     *
     * 关闭日志文件
     */
    ~Logger();

    /**
     * @brief 获取当前时间戳字符串
     *
     * @return std::string 格式化后的时间字符串 (YYYY-MM-DD HH:MM:SS)
     */
    std::string get_timestamp() const;

    /**
     * @brief 将日志级别转换为字符串
     *
     * @param level 日志级别
     * @return std::string 级别对应的字符串
     */
    std::string level_to_string(LogLevel level) const;

    /**
     * @brief 内部日志写入方法
     *
     * @param level 日志级别
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源代码行号
     */
    void write_log(LogLevel level, const std::string &message,
                   const std::string &file = "", int line = 0);

  public:
    /**
     * @brief 禁止拷贝和移动
     */
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    /**
     * @brief 获取单例实例
     *
     * @return Logger& 日志实例的引用
     */
    static Logger &get_instance();

    /**
     * @brief 记录日志
     *
     * @param level 日志级别
     * @param message 日志消息
     * @param file 源文件名 (可选)
     * @param line 源代码行号 (可选)
     */
    void log(LogLevel level, const std::string &message,
             const std::string &file = "", int line = 0);

    /**
     * @brief 记录 FileErrorCode 相关错误
     *
     * 将 FileErrorCode 转换为可读的错误消息并记录
     *
     * @param error 文件错误码
     * @param context 错误发生的上下文信息
     * @param file 源文件名 (可选)
     * @param line 源代码行号 (可选)
     */
    void log_error(FileErrorCode error, const std::string &context,
                   const std::string &file = "", int line = 0);

    /**
     * @brief 设置最低记录级别
     *
     * 只有大于等于此级别的日志才会被记录
     *
     * @param level 日志级别
     */
    void set_level(LogLevel level);

    /**
     * @brief 设置日志文件路径
     *
     * @param path 日志文件路径
     */
    void set_log_file(const std::string_view &path);

    /**
     * @brief 设置是否启用控制台输出
     *
     * @param enable true 启用，false 禁用
     */
    void set_console_output(bool enable);

    /**
     * @brief 设置是否启用文件输出
     *
     * @param enable true 启用，false 禁用
     */
    void set_file_output(bool enable);

    /**
     * @brief 刷新日志缓冲区
     */
    void flush();
};

// 便捷宏定义，简化日志调用
#define LOG_DEBUG(msg)                                                         \
    Logger::get_instance().log(LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)                                                          \
    Logger::get_instance().log(LogLevel::INFO, msg, __FILE__, __LINE__)
#define LOG_WARNING(msg)                                                       \
    Logger::get_instance().log(LogLevel::WARNING, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg)                                                         \
    Logger::get_instance().log(LogLevel::ERROR, msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg)                                                      \
    Logger::get_instance().log(LogLevel::CRITICAL, msg, __FILE__, __LINE__)

// FileErrorCode 专用日志宏
#define LOG_FILE_ERROR(error, context)                                         \
    Logger::get_instance().log_error(error, context, __FILE__, __LINE__)
