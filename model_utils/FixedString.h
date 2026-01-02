#pragma once
#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

template <size_t N> class FixedString {
  private:
    void assign(std::string_view str) {
        size_t copyLen = std::min(str.length(), N - 1);

        if (copyLen > 0) {
            std::memcpy(data, str.data(), copyLen);
        }

        data[copyLen] = '\0';
    }

  public:
    char data[N];

    // 默认构造
    FixedString() { data[0] = '\0'; }

    FixedString(const char *str) {
        assign(str); // 或者 assign(std::string_view(str));
    }

    FixedString(std::string_view s) { assign(s); }

    size_t length() const { return std::string_view(data).length(); }
    size_t size() const { return length(); }
    bool empty() const { return data[0] == '\0'; }
    static constexpr size_t capacity() { return N; }

    // 赋值运算符也只需要一个
    FixedString &operator=(std::string_view s) {
        assign(s);
        return *this;
    }

    // 隐式转换为 string_view
    operator std::string_view() const {
        return std::string_view(data, length());
    }

    // 支持加法运算符： fs += "hello"
    friend std::string operator+(const FixedString<N> &fs,
                                 std::string_view str) {
        return std::string(static_cast<std::string_view>(fs)) +
               std::string(str);
    }
};
