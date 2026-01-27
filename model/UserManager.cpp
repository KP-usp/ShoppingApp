/**
 * @file      UserManager.cpp
 * @brief     用户管理模块实现文件
 * @details   实现了 User 类和 UserManager 类的核心逻辑，
 *            包括注册校验、登录验证（哈希比对）、
 *             负责处理储存在 MySQL 表单的用户数据的CRUD 操作。
 * @author    KP-usp
 * @date      2025-01-23
 * @version   2.0
 * @copyright Copyright (c) 2025
 */

#include "UserManager.h"
#include "Database.h"
#include <ctime>
#include <fstream>
#include <optional>
#include <string>

using std::ifstream;
using std::nullopt;
using std::ofstream;
using std::optional;
using std::string;

Result UserManager::check_login(const string &username,
                                const string &input_password) {
    // 这里 user  的状态已经是正常的
    optional<User> user_opt = get_user_by_name(username);

    if (user_opt.has_value()) {
        User user = user_opt.value();

        if (check_password(input_password, string(user.password)) ==
            Result::SUCCESS) {
            active_user =
                std::make_shared<User>(user.username, user.password,
                                       user.is_admin, user.id, user.status);
            Logger::LOG_INFO("用户登录成功，用户名: " + username);
            return Result::SUCCESS;
        }
    }

    Logger::LOG_INFO("用户登录失败，用户名: " + username);
    return Result::FAILURE;
}

Result UserManager::is_valid_username_format(const string &username,
                                             string &error_message) {
    if (username.empty()) {
        error_message = "用户名不能为空";
        return Result::FAILURE;
    } else if (username.size() > User::MAX_USERNAME_SIZE) {
        error_message = "用户名过长";
        return Result::FAILURE;
    } else if (username.length() < User::MIN_USERNAME_SIZE) {
        error_message = "用户名过短";
        return Result::FAILURE;
    } else {
        error_message = "";
        return Result::SUCCESS;
    }
}

Result UserManager::is_valid_password_format(const string &password,
                                             string &error_message) {

    if (password.empty()) {
        error_message = "密码不能为空";
        return Result::FAILURE;
    } else if (password.size() > User::MAX_PASSWORD_SIZE) {
        error_message = "密码过长";
        return Result::FAILURE;
    } else if (password.length() < User::MIN_PASSWORD_SIZE) {
        error_message = "密码过短";
        return Result::FAILURE;
    } else
        return Result::SUCCESS;
}

Result UserManager::check_register(const string &username,
                                   const string &password,
                                   const string &again_password,
                                   string &error_message) {
    if (is_valid_username_format(username, error_message) == Result::FAILURE ||
        is_valid_password_format(password, error_message) == Result::FAILURE)
        return Result::FAILURE;
    if (get_user_by_name(username).has_value()) {
        error_message = "用户名已存在";
        return Result::FAILURE;
    }

    if (password != again_password) {
        error_message = "两次输入的密码不一致";
        return Result::FAILURE;
    }

    string hash_password = SecurityUtils::hash_password(password);

    // 添加加密密码的用户
    User temp(username, hash_password, false);
    append_user(temp);

    Logger::LOG_INFO("用户注册成功，用户名: " + username);
    return Result::SUCCESS;
}

void UserManager::append_user(const User &new_user) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法添加新用户。");
    }

    string username = string(new_user.username);
    string password = string(new_user.password);
    bool is_admin = new_user.is_admin;

    try {
        std::string sql = "INSERT INTO users (username, password, is_admin) "
                          "VALUES(?, ?, ?)";
        Database::prepare(sql, [&username, &password,
                                &is_admin](sql::PreparedStatement *pstmt) {
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            pstmt->setBoolean(3, is_admin);
            pstmt->executeUpdate();
        });
    } catch (sql::SQLException &e) {
        LOG_ERROR("添加新用户失败: " + std::string(e.what()));
    }
}

void UserManager::update_user(const int id, const string username,
                              const string password, const bool is_admin) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法更新用户。");
    }

    try {
        Database::prepare(
            "UPDATE users SET username = ?, password = ?, is_admin = ? WHERE "
            "id = ?",
            [&username, &password, &is_admin,
             &id](sql::PreparedStatement *pstmt) {
                pstmt->setString(1, username);
                pstmt->setString(2, password);
                pstmt->setBoolean(3, is_admin);
                pstmt->setInt(4, id);
                pstmt->executeUpdate();
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("更新用户失败: " + std::string(e.what()));
    }
}

optional<User> UserManager::get_user_by_name(const string &username) {
    auto con = Database::get_connection();
    if (!con)
        LOG_ERROR("数据库未连接，无法获取用户信息。");

    // 查询结果
    optional<User> result = nullopt;

    try {
        Database::prepare(
            "SELECT id, username, password, is_admin FROM users "
            "WHERE username = ? ",
            [&](sql::PreparedStatement *pstmt) {
                pstmt->setString(1, username);
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                if (res->next()) {
                    User temp;
                    temp.id = res->getInt("id");
                    temp.username = string(res->getString("username"));
                    temp.password = string(res->getString("password"));
                    temp.is_admin = res->getBoolean("is_admin");

                    result = temp;
                }
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("根据用户名获取用户信息失败，" + string(e.what()));
    }

    return result;
}

std::optional<User> UserManager::get_user_by_id(const int user_id) {
    auto con = Database::get_connection();
    if (!con)
        LOG_ERROR("数据库未连接，无法获取用户信息。");

    // 查询结果
    optional<User> result = nullopt;

    try {
        std::string sql = "SELECT id, username, password, is_admin FROM users "
                          "WHERE id = ?";
        Database::prepare(sql, [&](sql::PreparedStatement *pstmt) {
            pstmt->setInt(1, user_id);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) {
                User temp;
                temp.id = res->getInt("id");
                temp.username = string(res->getString("username"));
                temp.password = string(res->getString("password"));
                temp.is_admin = res->getBoolean("is_admin");

                result = temp;
            }
        });
    } catch (sql::SQLException &e) {
        LOG_ERROR("根据用户 id 获取用户信息失败，" + string(e.what()));
    }

    return result;
}

std::vector<User> UserManager::search_users_list(const string &query) {
    auto con = Database::get_connection();
    if (!con)
        LOG_ERROR("数据库未连接，无法搜索用户列表。");

    // 搜索结果
    std::vector<User> result;

    try {
        Database::prepare(
            "SELECT id, username, password, is_admin, status FROM users",
            [&query, &result](sql::PreparedStatement *pstmt) {
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                while (res->next()) {
                    User temp;

                    if (query.empty()) {
                        temp.id = res->getInt("id");
                        temp.username = res->getString("username");
                        temp.password = res->getString("password");
                        temp.is_admin = res->getBoolean("is_admin");
                        temp.status =
                            static_cast<UserStatus>(res->getInt("status"));
                        result.push_back(temp);
                        continue;
                    }

                    string low_query = query;

                    std::transform(low_query.begin(), low_query.end(),
                                   low_query.begin(), ::tolower);

                    temp.id = res->getInt("id");
                    temp.username = string(res->getString("username"));
                    temp.password = string(res->getString("password"));
                    temp.is_admin = res->getBoolean("is_admin");
                    temp.status =
                        static_cast<UserStatus>(res->getInt("status"));

                    string user_id_str = std::to_string(temp.id);
                    string username_str = string(temp.username);

                    if (user_id_str == low_query) {
                        result.push_back(temp);
                        break;
                    }

                    if (username_str.find(low_query) != string::npos) {
                        result.push_back(temp);
                    }
                }
            });
    } catch (sql::SQLException &e) {
        LOG_ERROR("搜索用户列表失败，" + string(e.what()));
    };
    return result;
}

void UserManager::delete_user(const int user_id) {
    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法删除用户。");
    }

    try {

        Database::prepare("UPDATE users SET status = ? WHERE id = ?",
                          [&user_id](sql::PreparedStatement *pstmt) {
                              pstmt->setInt(
                                  1, static_cast<int>(UserStatus::DELETED));
                              pstmt->setInt(2, user_id);
                              pstmt->executeUpdate();
                          });
    } catch (sql::SQLException &e) {
        LOG_ERROR("删除用户失败: " + std::string(e.what()));
    }
}

void UserManager::restore_user(const int user_id) {

    if (!Database::is_connected()) {
        LOG_ERROR("数据库未连接，无法恢复用户。");
    }

    try {

        Database::prepare("UPDATE users SET status = ? WHERE id = ?",
                          [&user_id](sql::PreparedStatement *pstmt) {
                              pstmt->setInt(1, 0);
                              pstmt->setInt(2, user_id);
                              pstmt->executeUpdate();
                          });
    } catch (sql::SQLException &e) {
        LOG_ERROR("恢复用户失败: " + std::string(e.what()));
    }
}
