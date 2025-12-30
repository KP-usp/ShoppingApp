#pragma once

#include "FixedString.h"
#include "Result.h"
#include "Role.h"

class UserManager;

class User { // 用户类
    friend UserManager;

  public:
    static constexpr size_t MAX_USERNAME_SIZE = 64 + 1;
    static constexpr size_t MAX_PASSWORD_SIZE = 128 + 1;
    enum class USER_STATUS {
        USER_NORMAL = 0,
        USER_DELETED = -1,
    };

  private:
    using string_view = std::string_view;

    int m_id;
    FixedString<MAX_USERNAME_SIZE> m_username;
    FixedString<MAX_PASSWORD_SIZE> m_password;
    Role m_userrole;
    USER_STATUS m_flag; // 用户状态

  public:
    // 构造函数
    User(const int id, const string_view &name, const string_view &password,
         const Role &role, USER_STATUS flag = USER_STATUS::USER_NORMAL);
    User(const string_view &name) { this->m_username = name; };
    User() {};

    // 获取用户名
    FixedString<MAX_USERNAME_SIZE> get_username() const { return m_username; }

    // 获取用户唯一凭证
    int get_id() const { return m_id; }

    // 判断是否删除（有标志位）
    Result is_deleted() const {
        if (this->m_flag == USER_STATUS::USER_DELETED) {
            return Result::SUCCESS;
        } else {
            return Result::FAILURE;
        }
    }

    // 获得用户权限名
    Role get_role() const { return m_userrole; }
};
