#include "FixedString.h"

class Role { // 角色权限类：用户、管理员
  public:
    static constexpr size_t MAX_ROLENAME_SIZE = 64 + 1;

  private:
    FixedString<MAX_ROLENAME_SIZE> m_rolename;
    int m_flag;

  public:
    Role(const FixedString<MAX_ROLENAME_SIZE> &rolename = "guest",
         int roleFlag = 0)
        : m_rolename(rolename), m_flag(roleFlag) {}
    const FixedString<MAX_ROLENAME_SIZE> &get_rolename() const {
        return m_rolename;
    }
};
