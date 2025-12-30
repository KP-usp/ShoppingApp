#pragma once
#include "CartManager.h"
#include "OrderManager.h"
#include "ProductManager.h"
#include "UserManager.h"

struct AppContext {
    // std::mutex db_mutex;
    UserManager user_manager;
    ProductManager product_manager;
    CartManager cart_manager;
    OrderManager order_manager;

    // 全局 UI 状态
    User *current_user = nullptr; // 指向当前登入用户的指针，空则未登入

    // 复制构造函数
    AppContext(const std::string_view &user_db_filename,
               const std::string_view &product_db_filename,
               const std::string_view &cart_db_filename,
               const std::string_view &order_db_filename)
        : user_manager(user_db_filename), product_manager(product_db_filename),
          cart_manager(cart_db_filename), order_manager(order_db_filename) {}

    // 赋值函数
    AppContext &operator=(const AppContext &&) = delete;
    AppContext &operator=(const AppContext &) = delete;

    AppContext(AppContext &&) = delete;
    AppContext &operator=(AppContext &&) = delete;
};
