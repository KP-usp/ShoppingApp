#pragma once

#include "FileError.h"
#include "FixedString.h"
#include "Result.h"
#include <Utils.h>
#include <memory>
#include <optional>
#include <string>

enum class UserStatus {
    USER_NORMAL = 0,
    USER_DELETED = -1,
};

struct Role {
    static constexpr int ADMIN_NAME_SIZE = 5 + 1;
    static constexpr int USER_NAME_SIZE = 4 + 1;
    FixedString<ADMIN_NAME_SIZE> ADMIN = "admin";
    FixedString<USER_NAME_SIZE> USER = "user";
};

class User {
  public:
    static constexpr size_t MAX_USERNAME_SIZE = 16 + 1;
    static constexpr size_t MAX_ROLENAME_SIZE = 6 + 1;
    static constexpr size_t MAX_PASSWORD_SIZE = 16 + 1;
    static constexpr size_t MIN_USERNAME_SIZE = 2 + 1;
    static constexpr size_t MIN_PASSWORD_SIZE = 5 + 1;

    int id;

    FixedString<MAX_USERNAME_SIZE> username;

    FixedString<MAX_PASSWORD_SIZE> password;

    FixedString<MAX_ROLENAME_SIZE> rolename;

    UserStatus status;

    User() {};
    User(const std::string_view &username, const std::string_view &password,
         const std::string_view &rolename, const int user_id = -1,
         const UserStatus status = UserStatus::USER_NORMAL)
        : id(user_id), status(status) {
        this->username = username;
        this->password = password;
        this->rolename = rolename;
    };
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
    std::unique_ptr<User> active_user = nullptr;

    // 初始化数据库，向数据库写入文件头（方便为用户生成 ID）
    void init_db_file();

    // 生成 ID 并更新文件
    int generate_and_update_id();

    // 辅助函数：查询用户对象块在数据库文件中的位置
    std::optional<streampos> get_user_pos(int id);

    // 辅助函数: 根据用户对象的用户名获取 id
    int get_id_by_username(const string_view &username); // ?

    // 添加删除标志
    void mark_deleted(User &user) { user.status = UserStatus::USER_DELETED; }

    // 辅助函数：验证密码
    Result check_password(const string_view &input_password,
                          const string_view &stored_password) {
        if (input_password == stored_password)
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
    std::optional<User> check_login(const string_view &username,
                                    const string_view &password);

    static Result is_valid_username_format(const string &username,
                                           string &error_message);

    static Result is_valid_password_format(const string &password,
                                           string &error_message);

    Result check_register(const string &username, const string &password,
                          const string &again_password, string &error_message);

    // 获取当前登入的用户
    User *get_current_user() { return active_user.get(); }

    // 当前用户注销
    void logout() { active_user = nullptr; }

    // 用户添加
    FileErrorCode append_user(const User &new_user);

    // 用户更新（用户名|密码）
    FileErrorCode update_user(const User &new_user);

    // 查找单个用户并返回
    std::optional<User> get_user(const string_view &username);

    // 用户模糊检索
    int search_user(const string_view &username);

    // 删除用户（使用删除标志）
    FileErrorCode delete_user(int id);

    // 析构器
    ~UserManager() {};
};
