#pragma once
#include "Logger.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <memory>
#include <mutex>
#include <mysql_connection.h>
#include <mysql_driver.h>

// 基本连接参数
struct DbConfig {
    std::string host = "localhost";
    std::string user = "root";
    std::string password;
    std::string database;
    bool enable_local_infile = true;
};

// 数据库连接类
class Database {
  private:
    // 驱动
    static sql::mysql::MySQL_Driver *driver;

    // 连接
    static std::unique_ptr<sql::Connection> con;

    // 是否添加表结构
    static bool is_tables_initialized;

    // 线程安全锁
    static std::mutex mutex;

    // 初始化表结构
    static void initialize_tables();

    // 单例模式
    Database() = default;

  public:
    static bool connect(const DbConfig &config);

    // 获取连接
    static sql::Connection *get_connection();

    // 检查是否链接
    static bool is_connected();

    // 关闭连接
    static void close();

    // 辅助函数：自动管理 Statement 对象生命周期
    static bool execute(const std::string &query);

    // 执行查询（INSERT/UPDATE/DELETE）
    template <typename Func>
    static void query(const std::string &sql, Func &&callback);

    // 执行预编译语句（防 SQL 注入）
    template <typename Func>
    static void prepare(const std::string &sql, Func &&callback);
};

// 模板函数实现
template <typename Func>
void Database::query(const std::string &sql, Func &&callback) {
    if (!is_connected()) {
        throw std::runtime_error("数据库未连接，请先调用 connect()");
    }

    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(sql));

    while (res->next()) {
        callback(res.get());
    }
}

template <typename Func>
void Database::prepare(const std::string &sql, Func &&callback) {
    if (!is_connected()) {
        throw std::runtime_error("数据库未连接，请先调用 connect()");
    }
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(sql));
        callback(pstmt.get());
    } catch (sql::SQLException &e) {
        LOG_ERROR("SQL 预处理失败: " + sql + " 错误原因: " + e.what());
        throw;
    }
}
