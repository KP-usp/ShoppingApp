#pragma once

#include "FileError.h"
#include "FixedString.h"
#include "Result.h"
#include "SecurityUtils.h"
#include <Utils.h>
#include <memory>
#include <optional>
#include <string>

enum class UserStatus {
    NORMAL = 0,
    DELETED = -1,
};

class User {
  public:
    static constexpr size_t MAX_USERNAME_SIZE = 16 + 1;
    static constexpr size_t MAX_ROLENAME_SIZE = 6 + 1;
    static constexpr size_t MAX_PASSWORD_SIZE = 16 + 1;
    static constexpr size_t MIN_USERNAME_SIZE = 2 + 1;
    static constexpr size_t MIN_PASSWORD_SIZE = 5 + 1;
    static constexpr size_t HASH_PASSWORD_SIZE = 97 + 1;

    int id;

    FixedString<MAX_USERNAME_SIZE> username;

    FixedString<HASH_PASSWORD_SIZE> password;

    bool is_admin;

    UserStatus status;

    User() {};
    User(const std::string_view &username, const std::string_view &password,
         const bool role_opt = false, const int user_id = -1,
         const UserStatus status = UserStatus::NORMAL)
        : id(user_id), username(username), password(password),
          is_admin(role_opt), status(status) {};
};

class UserManager { // 管理用户类
  public:
    static constexpr size_t MAX_FILENAME_LENGTH = 64 + 1;

  private:
    using string_view = std::string_view;
    using streampos = std::streampos;
    using string = std::string;

    // 存储 User 数据库文件
    string m_db_filename;

    // 当前激活的用户
    std::shared_ptr<User> active_user = nullptr;

    // 初始化数据库，向数据库写入文件头（方便为用户生成 ID）
    void init_db_file();

    // 生成 ID 并更新文件
    int generate_and_update_id();

    // 辅助函数：查询用户对象块在数据库文件中的位置
    std::optional<streampos> get_user_pos(const int id);

    // 辅助函数: 根据用户对象的用户名获取 id
    int get_id_by_username(const string_view &username); // ?

    // 辅助函数：验证密码
    Result check_password(const string &input_password,
                          const string &stored_password) {
        if (SecurityUtils::check_password(input_password, stored_password))
            return Result::SUCCESS;
        else
            return Result::FAILURE;
    }

  public:
    // 构造器
    UserManager(const string_view &db_filename) : m_db_filename(db_filename) {
        init_db_file();
    }

    // 用户登入验证
    Result check_login(const string &username, const string &password);

    static Result is_valid_username_format(const string &username,
                                           string &error_message);

    static Result is_valid_password_format(const string &password,
                                           string &error_message);

    Result check_register(const string &username, const string &password,
                          const string &again_password, string &error_message);

    // 获取当前登入的用户
    std::shared_ptr<User> get_current_user() { return active_user; }

    // 当前用户注销
    void logout() { active_user = nullptr; }

    // 用户添加
    FileErrorCode append_user(const User &new_user);

    // 用户更新（用户名|密码）
    FileErrorCode update_user(const User &new_user);

    // 查找单个用户并返回
    std::optional<User> get_user_by_name(const string_view &username);

    // 根据用户 ID 查找用户
    std::optional<User> get_user_by_id(const int user_id);

    // 用户模糊检索
    int search_user(const string_view &username);

    // 用于关键词匹配用户（支持 ID 精确查找和用户名模糊查找）
    std::vector<User> search_users_list(const string &query);

    // 删除用户（使用删除标志）
    FileErrorCode delete_user(const int id);

    // 恢复用户(删除 -> 正常)
    FileErrorCode restore_user(const int id);

    // 析构器
    ~UserManager() {};
};
