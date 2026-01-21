#pragma once
#include <cstring>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>

class SecurityUtils {
  private:
    // 相关常量
    static constexpr int ITERATIONS = 100000; // 迭代次数
    static constexpr int KEY_LENGTH = 32;     // 生成的 Hash 长度（256 bit）
    static constexpr int SALT_LENGTH = 16;    // 盐的长度（128 bit）

    // 辅助函数：二进制转十六进制字符串
    static std::string bin_to_hex_str(const std::vector<unsigned char> &data);

    // 辅助函数：十六进制字符转二进制
    static std::vector<unsigned char> hex_str_to_bin(const std::string &hex);

    // 执行 PBKDF2 算法
    static std::vector<unsigned char>
    pbkdf2(const std::string &password, const std::vector<unsigned char> &salt);

    // 恒定时间比较函数(防止时序攻击)
    static bool constant_time_compare(const std::vector<unsigned char> &a,
                                      const std::vector<unsigned char> &b);

  public:
    // 加密密码（注册时使用）
    static std::string hash_password(const std::string &password);

    static bool check_password(const std::string &password,
                               const std::string &stored_value);
};
