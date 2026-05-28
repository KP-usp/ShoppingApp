#pragma once
#include "Logger.h"
#include <memory>
#include <mutex>
#include <mysqlx/xdevapi.h>

struct DbConfig {
    std::string host = "localhost";
    int port = 33060;
    std::string user = "root";
    std::string password;
    std::string database;
};

class Database {
  private:
    static std::unique_ptr<mysqlx::Session> session;
    static bool is_connected_flag;
    static bool is_tables_initialized;
    static std::mutex mutex;

    static void initialize_tables();

    Database() = default;

  public:
    static bool connect(const DbConfig &config);

    static mysqlx::Session& get_session();

    static bool is_connected();

    static void close();

    static bool execute(const std::string &query);

    template <typename Func>
    static void query(const std::string &sql, Func &&callback) {
        if (!is_connected_flag) {
            throw std::runtime_error("数据库未连接，请先调用 connect()");
        }
        try {
            auto result = session->sql(sql).execute();
            while (auto row = result.fetchOne()) {
                callback(row);
            }
        } catch (const mysqlx::Error &e) {
            LOG_ERROR("SQL 查询失败: " + sql + " 错误: " + e.what());
            throw;
        }
    }
};
