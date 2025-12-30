#pragma once

#include <iomanip>
#include <memory> // for std::unique_ptr
#include <sstream>
#include <stdexcept> // for std::runtime_error
#include <string>
#include <vector>

// OpenSSL headers
#include <openssl/evp.h>
#include <openssl/rand.h>

class SecurityUtils {
  public:
    // 删除默认构造，因为这是纯工具类
    SecurityUtils() = delete;

    /**
     * 生成随机盐 (Salt)
     * @param byteLength 盐的字节长度 (默认16字节，转为Hex后为32字符)
     */
    static std::string generateSalt(int byteLength = 16) {
        std::vector<unsigned char> buffer(byteLength);

        // 2. 增加错误检查：如果随机数生成失败，必须抛出异常
        if (RAND_bytes(buffer.data(), byteLength) != 1) {
            throw std::runtime_error(
                "OpenSSL RAND_bytes failed: Not enough entropy.");
        }

        std::stringstream ss;
        for (int i = 0; i < byteLength; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << (int)buffer[i];
        }
        return ss.str();
    }

    /**
     * SHA256 哈希计算 (使用 EVP 接口，兼容 OpenSSL 3.0)
     */
    static std::string sha256(const std::string &str) {
        // 3. 使用 std::unique_ptr 自动管理 EVP_MD_CTX 的内存
        // 自定义删除器：当指针销毁时，自动调用 EVP_MD_CTX_free
        using EVP_CTX_Ptr =
            std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
        EVP_CTX_Ptr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);

        if (!context) {
            throw std::runtime_error("Failed to create EVP_MD_CTX");
        }

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int lengthOfHash = 0;

        // 4. 执行哈希计算 (带错误检查)
        if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
            EVP_DigestUpdate(context.get(), str.c_str(), str.size()) != 1 ||
            EVP_DigestFinal_ex(context.get(), hash, &lengthOfHash) != 1) {
            throw std::runtime_error("OpenSSL EVP hashing failed");
        }

        std::stringstream ss;
        for (unsigned int i = 0; i < lengthOfHash; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    // 最终存储的生成函数
    static std::pair<std::string, std::string>
    encryptPassword(const std::string &password) {
        std::string salt = generateSalt();
        std::string combined = password + salt;
        std::string hashedPassword = sha256(combined);
        return {hashedPassword, salt};
    }

    // 验证密码
    static bool verifyPassword(const std::string &inputPassword,
                               const std::string &storedHash,
                               const std::string &storedSalt) {
        std::string combined = inputPassword + storedSalt;
        std::string inputHash = sha256(combined);
        // 这里可以直接比较字符串，C++ string 重载了 == 操作符
        return inputHash == storedHash;
    }
};
