#include "User.h"
using std::string_view;

// 构造函数
User::User(const int id, const string_view &name, const string_view &password,
           const Role &role, USER_STATUS flag)
    : m_userrole(role), m_flag(flag) {
    this->m_username = name;
    this->m_password = password;
};
