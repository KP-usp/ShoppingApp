#pragma once
#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

template <size_t N> class FixedString {
  private:
    using string_view = std::string_view;

    // 字面量转换为字符数组
    void assign(string_view str) {
        std::memset(data, 0, N);
        size_t copyLen = std::min(str.length(), N - 1);
        std::memcpy(data, str.data(), copyLen);
    }

  public:
    char data[N];

    // 默认构造：清零
    FixedString() { std::memset(data, 0, N); }

    // 支持从字符串字面量构造
    FixedString(const char *str) { assign(str); }

    // 测量字符长度
    size_t length() const { return string_view(data).length(); }
    // 与上面作用相同
    size_t size() const { return length(); }
    // 判断是否为空
    bool empty() const { return data[0] == '\0'; }
    // 最大容量（编译器常量）
    static constexpr size_t capacity() { return N; }

    // 支持赋值运算符： fs = "world";
    FixedString &operator=(string_view str) {
        assign(str);
        return *this;
    }
    // 支持加法运算符： fs += "hello"
    friend std::string operator+(const FixedString<N> &fs,
                                 std::string_view str) {
        // 利用 string_view 隐式转换和 std::string 的 operator+
        return std::string(static_cast<std::string_view>(fs)) +
               std::string(str);
    }

    // 隐式转换为 string_view, 方便打印和比较
    operator string_view() const { return string_view(data); }
};
