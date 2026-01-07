/**
 * @file      UserManager.h
 * @brief     用户管理模块头文件
 * @details   定义了用户类(User)和用户管理类(UserManager),
 * 负责用户的注册、登录校验及数据持久化。
 */

#pragma once
#include "FileError.h"
#include "FixedString.h"
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

/**
 * @brief 用户管理类
 *
 * 维护用户的状态，提供用户注册、登录验证、用户注销、用户封禁、信息更新与获取等功能
 *
 */
class UserManager {
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

    // 查询用户对象块在数据库文件流中的位置(streampos)
    std::optional<streampos> get_user_pos(const int id);

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
     * @brief  初始化类对象和用户数据库文件
     *
     * 向文件写入文件头（携带 ID 数据, 方便为每一个用户生成 ID）
     *
     * @param db_filename 用户数据库文件名
     * @return   无返回值
     */
    UserManager(const string_view &db_filename) : m_db_filename(db_filename) {
        init_db_file();
    }

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
     * @return  Result 成功返回 Result::SUCCESS，失败返回 Result::FAILURE
     * @note 这里存储到数据库中密码是经过哈希加密后的密码
     */
    std::shared_ptr<User> get_current_user() { return active_user; }

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
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode append_user(const User &new_user);

    /**
     * @brief 更新用户信息
     *
     * 更新用户的信息（密码、角色等）
     *
     * @param new_user 用户对象
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode update_user(const User &new_user);

    /**
     * @brief 根据用户名获取用户对象
     *
     * @param  username 用户名
     * @return optional<User>  成功：用户对象（），失败：nullopt
     */
    std::optional<User> get_user_by_name(const string_view &username);

    /**
     * @brief 根据用户 id， 获取用户对象
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
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode delete_user(const int user_id);

    /**
     * @brief 根据用户 id，恢复数据库用户数据（添加正常标志）
     *
     * @param  user_id  用户 ID
     * @return FileErrorCode 成功返回 FileErrorCode::SUCCESS，失败返回相应错误码
     */
    FileErrorCode restore_user(const int user_id);

    // 析构器
    ~UserManager() {};
};
