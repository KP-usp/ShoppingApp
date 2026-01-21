#include "Database.h"
#include "Logger.h"
#include <stdexcept>

// 定义静态数据成员
sql::mysql::MySQL_Driver *Database::driver = nullptr;
std::mutex Database::mutex;
std::unique_ptr<sql::Connection> Database::con;
bool Database::is_tables_initialized = false;

bool Database::connect(const std::string &host, const std::string &user,
                       const std::string &password,
                       const std::string &database) {

    try {
        if (is_connected())
            return true; // 已连接则跳过

        // 获取驱动实例
        driver = sql::mysql::get_mysql_driver_instance();

        // 创建连接
        con.reset(driver->connect(host, user, password));

        // 选择数据库
        con->setSchema(database);

        // 初始化表结构
        initialize_tables();

        return true;
    } catch (sql::SQLException &e) {
        LOG_ERROR("MySQL 连接失败" + std::string(e.what()));
        return false;
    }
}

sql::Connection *Database::get_connection() {
    if (!is_connected()) {
        throw std::runtime_error("数据库未连接，请先调用 connect()");
    }
    return con.get();
}

bool Database::is_connected() { return con && con->isValid(); }

void Database::close() {
    std::lock_guard<std::mutex> lock(mutex);

    if (con) {
        con->close();
        con.reset();
    }
    is_tables_initialized = false;
    LOG_INFO("数据库连接已关闭");
}

void Database::initialize_tables() {
    // 如果已经初始化过表结构，直接返回
    if (is_tables_initialized)
        return;

    try {

        // 创建 users 表
        std::string create_users_table = R"(
            CREATE TABLE IF NOT EXISTS users (
                id INT PRIMARY KEY AUTO_INCREMENT,
                username VARCHAR(16) NOT NULL UNIQUE,
                password  VARCHAR(97) NOT NULL,
                is_admin BOOLEAN DEFAULT FALSE,
                status TINYINT DEFAULT 0,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_username (username),
                INDEX idx_status (status)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
             )";

        // 创建 products 表
        std::string create_products_table = R"(
              CREATE TABLE IF NOT EXISTS products (
              product_id INT PRIMARY KEY AUTO_INCREMENT,
              product_name VARCHAR(100) NOT NULL UNIQUE, 
              price DOUBLE NOT NULL,
              stock INT NOT NULL DEFAULT 0,
              status TINYINT DEFAULT 0,      
              created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
          ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
             )";

        // 创建 carts 表
        std::string create_carts_table = R"(
              CREATE TABLE IF NOT EXISTS cart_items (
              id INT AUTO_INCREMENT,          
              user_id INT NOT NULL,
              product_id INT NOT NULL,
              count INT NOT NULL DEFAULT 1,
              status TINYINT DEFAULT 0,       
              delivery_selection INT DEFAULT -1,
              created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
              PRIMARY KEY (id),
              INDEX idx_user_status (user_id, status),
              UNIQUE KEY uk_user_product (user_id, product_id)
              ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
            )";

        // 创建 orders 表
        std::string create_orders_table = R"(
              CREATE TABLE orders (
              id INT AUTO_INCREMENT,
              user_id INT NOT NULL,
              product_id INT NOT NULL,
              order_id BIGINT NOT NULL,      
              count INT NOT NULL,
              order_time TIMESTAMP NOT NULL,
              delivery_selection INT NOT NULL,
              address VARCHAR(50),
              status TINYINT DEFAULT 0,      
              PRIMARY KEY (id),
              INDEX idx_user_order (user_id, order_id),
              INDEX idx_order_id (order_id),
              INDEX idx_status_time (status, order_time)
              ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
          )";

        // 创建 history_orders 表
        std::string create_history_orders_table = R"(
              CREATE TABLE IF NOT EXISTS history_orders (
              id INT AUTO_INCREMENT,
              user_id INT NOT NULL,
              product_name VARCHAR(100) NOT NULL,  
              price DOUBLE NOT NULL,              
              order_id BIGINT NOT NULL,
              count INT NOT NULL,
              order_time TIMESTAMP NOT NULL,
              delivery_selection INT NOT NULL,
              address VARCHAR(50),
              status TINYINT DEFAULT 0,
              PRIMARY KEY (id),
              INDEX idx_user_history (user_id, order_id),
              INDEX idx_status_time (status, order_time)
              ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
            )";

        // 执行创建表单语句
        auto stmt = std::unique_ptr<sql::Statement>(con->createStatement());
        stmt->execute(create_users_table);
        stmt->execute(create_products_table);
        stmt->execute(create_carts_table);
        stmt->execute(create_orders_table);
        stmt->execute(create_history_orders_table);

        LOG_INFO("数据库表初始化完成");

        is_tables_initialized = true;

    } catch (sql::SQLException &e) {
        LOG_ERROR("表初始化错误：" + std::string(e.what()));
        throw;
    }
}
