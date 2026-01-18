#pragma once
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <memory>
#include <mysql_connection.h>
#include <mysql_driver.h>

// 数据库连接类
class Database {
  private:
    static std::unique_ptr<sql::mysql::MySQL_Driver> driver;
    static std::unique_ptr<sql::Connection> con;

    // 单例模式
    Database() = default;

  public:
    static bool connect(const std::string &host, const std::string &user,
                        const std::string &password,
                        const std::string &database);

    static sql::Connection *getConnection();
    static void close();
};
