/**
 * @file      UserManager.h
 * @brief     用户管理模块头文件
 * @details   定义了用户类(User)和用户管理类(UserManager),
 * 负责用户的注册、登录校验及数据持久化。
 */

#pragma once

#include "Logger.h"
#include "Result.h"
#include "SecurityUtils.h"
#include <Utils.h>
#include <memory>
#include <optional>
#include <string>

/**
 * @brief 用户状态枚举
 *
 * 存储用户状态，包括正常和删除两种状态
 *
 */
enum class UserStatus {
    NORMAL = 0,   // 正常状态
    DELETED = -1, // 删除状态
};

/**
 * @brief 用户类
 *
 * 存储用户的基本信息，包括用户名，密码（哈希后），角色（管理员或普通用户）及状态（正常或删除）
 *
 */
class User {
  public:
    static constexpr size_t MAX_USERNAME_SIZE = 16 + 1;
    static constexpr size_t MAX_ROLENAME_SIZE = 6 + 1;
    static constexpr size_t MAX_PASSWORD_SIZE = 16 + 1;
    static constexpr size_t MIN_USERNAME_SIZE = 2 + 1;
    static constexpr size_t MIN_PASSWORD_SIZE = 5 + 1;
    static constexpr size_t HASH_PASSWORD_SIZE = 97 + 1;

    int id;

    std::string username;

    std::string password;

    bool is_admin;

    UserStatus status;

    User() {};

    User(const std::string_view &username, const std::string_view &password,
         const bool role_opt = false, const int user_id = -1,
         const UserStatus status = UserStatus::NORMAL)
        : id(user_id), username(username), password(password),
          is_admin(role_opt), status(status) {};
};

/**
 * @brief 用户管理类
 *
 * 维护用户的状态，提供用户注册、登录验证、用户注销、用户封禁、信息更新与获取等功能
 *
 */
class UserManager {

  private:
    using string_view = std::string_view;
    using streampos = std::streampos;
    using string = std::string;

    // 当前激活的用户
    std::shared_ptr<User> active_user = nullptr;

    // 检查用户名格式
    Result is_valid_username_format(const string &username,
                                    string &error_message);

    // 辅助函数：验证密码(比较数据库中哈希加密后的密码)
    Result check_password(const string &input_password,
                          const string &stored_password) {
        if (SecurityUtils::check_password(input_password, stored_password))
            return Result::SUCCESS;
        else
            return Result::FAILURE;
    }

  public:
    /**
     * @brief  初始化类对象
     *
     * @param   无参数
     * @return   无返回值
     */
    UserManager() = default;

    /**
     * @brief 验证用户登录
     *
     * 检查给定的用户名和密码是否匹配数据库中的记录。
     *
     * @param username 待验证的用户名
     * @param password 待验证的密码（明文）
     * @return  Result 成功返回 Result::SUCCESS，失败返回 Result::FAILURE
     * @note 这里密码是和数据库中存储的哈希加密密码进行比对的
     */
    Result check_login(const string &username, const string &password);

    /**
     * @brief 验证用户注册
     *
     * 检查用户名和密码是否符合一定的格式要求，并确保两次输入的密码一致。
     *
     * @param username 待验证的注册用户名
     * @param password 待验证的密码（明文）
     * @param again_password 再次输入的密码（明文）
     * @param error_message 若验证失败，返回具体的错误信息
     * @return  Result 成功返回 Result::SUCCESS，失败返回 Result::FAILURE
     * @note 这里存储到数据库中密码是经过哈希加密后的密码
     */
    Result check_register(const string &username, const string &password,
                          const string &again_password, string &error_message);

    /**
     * @brief 用户密码格式验证
     *
     *
     *
     * @param password 待验证的密码（明文）
     * @param error_message 若验证失败，返回具体的错误信息
     * @return  Result 成功返回 Result::SUCCESS，失败返回 Result::FAILURE
     * @note 静态函数方便 UI 前端和类方法调用
     */
    static Result is_valid_password_format(const string &password,
                                           string &error_message);

    /**
     * @brief 获取当前激活的用户
     *
     * @param 无参数
     * @return  active_user 返回当前用户指针
     * @note 这里存储到数据库中密码是经过哈希加密后的密码
     */
    std::shared_ptr<User> get_current_user() {
        if (active_user)
            return active_user;
        else
            LOG_CRITICAL("当前 usermanager 没有持有用户");
        return nullptr;
    }

    /**
     * @brief 当前激活用户注销
     *
     * @param 无参数
     * @return  无返回值
     */
    void logout() { active_user = nullptr; }

    /**
     * @brief 添加新用户
     *
     * @param new_user 新用户对象
     * @return 无返回值
     */
    void append_user(const User &new_user);

    /**
     * @brief 更新用户信息
     *
     * 更新用户的信息（密码、角色等）
     *
     * @param id 用户 id
     * @param username 用户名
     * @param password 用户密码
     * @param is_admin 是否有管理权限
     * @return 无返回值
     */
    void update_user(const int id, const string username, const string password,
                     const bool is_admin);

    /**
     * @brief 根据用户名获取用户对象(正常状态)
     *
     * @param  username 用户名
     * @return optional<User>  成功：用户对象（），失败：nullopt
     */
    std::optional<User> get_user_by_name(const string &username);

    /**
     * @brief 根据用户 id， 获取用户对象（正常状态）
     *
     * @param  user_id  用户 ID
     * @return optional<User>  成功：用户对象（），失败：nullopt
     */
    std::optional<User> get_user_by_id(const int user_id);

    /**
     * @brief 根据查询词（用户名或 ID）匹配用户列表（支持 ID
     * 精确查找和用户名模糊查找）
     *
     * @param  query  查询词
     * @return vector<User> 返回的用户对象列表
     */
    std::vector<User> search_users_list(const string &query);

    /**
     * @brief 根据用户 id，删除用户（添加删除标志）
     *
     * 对商品进行软删除（修改状态为 DELETED），而非物理删除。
     *
     * @param  user_id  用户 ID
     * @return 无返回参数
     */
    void delete_user(const int user_id);

    /**
     * @brief 根据用户 id，恢复数据库用户数据（添加正常标志）
     *
     * @param  user_id  用户 ID
     * @return 无返回参数
     */
    void restore_user(const int user_id);

    // 析构器
    ~UserManager() {};
};
