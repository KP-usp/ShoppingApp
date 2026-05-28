#include "Database.h"
#include <stdexcept>

std::unique_ptr<mysqlx::Session> Database::session = nullptr;
bool Database::is_connected_flag = false;
bool Database::is_tables_initialized = false;
std::mutex Database::mutex;

bool Database::connect(const DbConfig &config) {
    try {
        if (is_connected())
            return true;

        session = std::make_unique<mysqlx::Session>(
            config.host, config.port, config.user, config.password,
            config.database);

        is_connected_flag = true;

        initialize_tables();

        LOG_INFO("数据库初始化并链接成功");
        return true;
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("MySQL 连接失败: " + std::string(e.what()));
        return false;
    }
}

mysqlx::Session& Database::get_session() {
    if (!is_connected_flag || !session) {
        throw std::runtime_error("数据库未连接，请先调用 connect()");
    }
    return *session;
}

bool Database::is_connected() { return is_connected_flag && session != nullptr; }

void Database::close() {
    std::lock_guard<std::mutex> lock(mutex);
    if (session) {
        session.reset();
    }
    is_connected_flag = false;
    is_tables_initialized = false;
    LOG_INFO("数据库连接已关闭");
}

bool Database::execute(const std::string &query) {
    if (!is_connected_flag) {
        LOG_ERROR("数据库未连接，无法执行语句。");
        return false;
    }
    try {
        session->sql(query).execute();
        return true;
    } catch (const mysqlx::Error &e) {
        LOG_ERROR("SQL 执行失败: " + query + " 错误: " + e.what());
        return false;
    }
}

void Database::initialize_tables() {
    if (is_tables_initialized)
        return;

    try {
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

        std::string create_carts_table = R"(
              CREATE TABLE IF NOT EXISTS carts (
              id INT AUTO_INCREMENT,
              user_id INT NOT NULL,
              product_id INT NOT NULL,
              count INT NOT NULL DEFAULT 1,
              status TINYINT DEFAULT 0,
              delivery_selection INT DEFAULT -1,
              created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
              PRIMARY KEY (id),
              INDEX idx_user_status (user_id, status),
              UNIQUE KEY uk_user_product (user_id, product_id, status)
              ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
            )";

        std::string create_orders_table = R"(
              CREATE TABLE IF NOT EXISTS orders (
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

        session->sql(create_users_table).execute();
        session->sql(create_products_table).execute();
        session->sql(create_carts_table).execute();
        session->sql(create_orders_table).execute();
        session->sql(create_history_orders_table).execute();

        LOG_INFO("数据库表初始化完成");

        is_tables_initialized = true;

    } catch (const mysqlx::Error &e) {
        LOG_ERROR("表初始化错误：" + std::string(e.what()));
        throw;
    }
}
