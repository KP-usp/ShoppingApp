#pragma once
#include <cstring>
#include <fstream>
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
    static std::string bin_to_hex_str(const std::vector<unsigned char> &data) {
        std::stringstream ss;
        for (unsigned char c : data) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        return ss.str();
    }

    // 辅助函数：十六进制字符转二进制
    static std::vector<unsigned char> hex_str_to_bin(const std::string &hex) {
        std::vector<unsigned char> data;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byte_str = hex.substr(i, 2);
            unsigned char byte =
                (unsigned char)strtol(byte_str.c_str(), nullptr, 16);
            data.push_back(byte);
        }
        return data;
    }

    // 执行 PBKDF2 算法
    static std::vector<unsigned char>
    pbkdf2(const std::string &password,
           const std::vector<unsigned char> &salt) {
        std::vector<unsigned char> hash(KEY_LENGTH);

        // 使用 OpenSSL 的 PKCS5_PBKDF2_HMAC 函数
        // 使用 Sha256 算法
        PKCS5_PBKDF2_HMAC(password.c_str(), password.length(), salt.data(),
                          salt.size(), ITERATIONS, EVP_sha256(), KEY_LENGTH,
                          hash.data());
        return hash;
    }

    // 恒定时间比较函数(防止时序攻击)
    static bool constant_time_compare(const std::vector<unsigned char> &a,
                                      const std::vector<unsigned char> &b) {
        if (a.size() != b.size()) {
            return false;
        }

        unsigned char result = 0;
        for (size_t i = 0; i < a.size(); ++i) {
            result |= a[i] ^ b[i];
        }
        return result == 0;
    }

  public:
    // 加密密码（注册时使用）
    static std::string hash_password(const std::string &password) {
        // 生成随机盐
        std::vector<unsigned char> salt(SALT_LENGTH);
        if (RAND_bytes(salt.data(), SALT_LENGTH) != 1) {
            throw std::runtime_error("OpenSSL random generation failed");
        }

        // 计算 hash
        std::vector<unsigned char> hash = pbkdf2(password, salt);

        // 拼接结果
        return bin_to_hex_str(salt) + "$" + bin_to_hex_str(hash);
    }

    static bool check_password(const std::string &password,
                               const std::string &stored_value) {
        // 解析字符串
        size_t delimiter_pos = stored_value.find('$');

        if (delimiter_pos == std::string::npos)
            return false;

        std::string salt_hex = stored_value.substr(0, delimiter_pos);
        std::string hash_hex = stored_value.substr(delimiter_pos + 1);

        std::vector<unsigned char> salt = hex_str_to_bin(salt_hex);
        std::vector<unsigned char> stored_hash = hex_str_to_bin(hash_hex);

        std::vector<unsigned char> new_hash = pbkdf2(password, salt);

        return constant_time_compare(new_hash, stored_hash);
    }
};
