#pragma once
#include "CartManager.h"
#include "HistoryOrderManager.h"
#include "OrderManager.h"
#include "ProductManager.h"
#include "UserManager.h"
#include <functional>

struct AppContext {
    UserManager user_manager;
    ProductManager product_manager;
    CartManager cart_manager;
    OrderManager order_manager;
    HistoryOrderManager history_order_manager;

    // 全局 UI 状态
    std::shared_ptr<User> current_user =
        nullptr; // 指向当前登入用户的指针，空则未登入

    // 回调函数，用来存放刷新屏幕的动作, 默认给个空实现
    std::function<void()> request_repaint = [] {};

    // 构造函数
    AppContext() = default;

    // 禁用赋值函数，采用单例
    AppContext &operator=(const AppContext &&) = delete;
    AppContext &operator=(const AppContext &) = delete;
    AppContext &operator=(AppContext &&) = delete;

    // 禁用移动构造函数，采用单例
    AppContext(AppContext &&) = delete;
};
