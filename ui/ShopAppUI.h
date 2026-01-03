#pragma once
#include "AppContext.h"
#include "CartPage.h"
#include "HistoryOrderPage.h"
#include "LoginPage.h"
#include "OrderPage.h"
#include "RegisterPage.h"
#include "SharedComponent.h"
#include "ShopPage.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include <atomic>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <thread>

using std::cout;
using std::endl;

using namespace ftxui;

class ShopAppUI {
  private:
    // app 管理类及当前使用者上下文
    AppContext &ctx;

    int tab_index = 0; // 当显示第几个页面：0-登录 1-注册
                       // 2-商城 3-购物车 4-订单

    // 页面类成员， 统一管理
    std::shared_ptr<LoginLayOut> login_layout;
    std::shared_ptr<RegisterLayOut> register_layout;
    std::shared_ptr<ShopLayOut> shop_layout;
    std::shared_ptr<CartLayOut> cart_layout;
    std::shared_ptr<OrderLayOut> order_layout;
    std::shared_ptr<HistoryOrderLayOut> history_order_layout;

    // 定义“外壳”，为了刷新商品页面
    Component shop_container_slot = Container::Vertical({});

    // 定义“外壳”, 专门用来放购物车页面,订单页面, 或历史订单页面
    // UI(需要登录后初始化)
    Component cart_container_slot = Container::Vertical({});
    Component order_container_slot = Container::Vertical({});
    Component history_order_container_slot = Container::Vertical({});

    Component main_container; // 整个 App 的主容器
    Component tab_container;  // 页面切换容器

    // 原子布尔值，用来控制后台线程何时停止
    std::atomic<bool> refresh_thread_running = false;

    // 全部的 lambda 回调函数
    std::function<void()> on_login;            // 前往登录页
    std::function<void()> on_login_success;    // 注册成功，返回登录页
    std::function<void()> on_register;         // 前往注册页
    std::function<void()> on_register_success; // 注册成功，返回登录页
    std::function<void()> on_logout_success;   // 注销成功，返回登录页
    std::function<void()> on_shopping;         // 前往商品页
    std::function<void()> on_checkout;         // 前往购物车
    std::function<void()> on_orders_info;      // 前往订单页
    std::function<void()> add_cart;            // 添加购物车商品时刷新 cart_page
    std::function<void()> delete_item_success; // 删除购物车商品刷新 cart_page
    std::function<void()> checkout_success;    // 结账刷新 cart_page
    std::function<void()> on_history_orders_info; // 前往历史订单页
    std::function<void()> on_orders_update;       // 修改或删除订单

  public:
    explicit ShopAppUI(AppContext &context);

    AppContext &get_context() { return ctx; }

    void run() {
        auto screen = ScreenInteractive::Fullscreen();

        // 任何持有 ctx 的页面都可以通过调用 ctx.request_repaint() 来刷新屏幕
        ctx.request_repaint = [&screen] { screen.Post(Event::Custom); };

        // 创建页面实例(cart_layout、order_layout、 history_order_layout
        // 都需要登录后创建)
        login_layout =
            std::make_shared<LoginLayOut>(ctx, on_login_success, on_register);
        register_layout = std::make_shared<RegisterLayOut>(
            ctx, on_register_success, on_login);
        shop_layout = std::make_shared<ShopLayOut>(ctx, on_checkout, add_cart);
        shop_container_slot->Add(shop_layout->get_component());

        // 使用 Tab 容器进行路由管理
        // 当 tab_index 变化时，显示的组件也会变化
        // 这里 cart_component 开始时是 “空壳”
        auto tab_content = Container::Tab(
            {
                login_layout->get_component(),
                register_layout->get_component(),
                shop_container_slot,
                cart_container_slot,
                order_container_slot,
                history_order_container_slot,
            },
            &tab_index);

        // 注销组件(放在导航栏中)
        auto btn_logout = Button("注销", [this] { on_logout_success(); });

        // 确保当前没用户时才激活注销组件
        auto logout_logic = Maybe(Container::Vertical({btn_logout}), [this] {
            return ctx.current_user != nullptr;
        });

        auto final_content = Container::Vertical({tab_content, logout_logic});

        //  全局导航栏 - 只有登录后才显示
        auto layout = Renderer(final_content, [&] {
            Element page = tab_content->Render();

            if (ctx.current_user == nullptr) {
                return page;
            }

            auto header_content = hbox(
                {text("  "),

                 hbox({text("🛒 ") | size(WIDTH, EQUAL, 2),
                       text("购物商城") | bold | color(Color::CyanLight)}) |
                     vcenter | flex,

                 hbox({filler(), text("🕒 ") | vcenter,
                       SharedComponents::get_clock_element() |
                           color(Color::White) | vcenter,
                       filler()}) |
                     flex,

                 hbox({filler(),
                       text("Hi, ") | dim | color(Color::GrayLight) | vcenter,
                       text(ctx.current_user->username + "") | bold |
                           color(Color::Gold1) | vcenter,
                       text("  ") | size(WIDTH, EQUAL, 2),
                       separator() | color(Color::GrayDark),
                       text("  ") | size(WIDTH, EQUAL, 1),
                       logout_logic->Render()}) |
                     flex,

                 text("  ")});

            auto header =
                vbox({header_content, separator() | color(Color::GrayDark)}) |
                bgcolor(Color::Grey11);

            return vbox({header, page | flex});
        });

        // 全局按键捕获 (Global Event Handler)
        // 处理导航快捷键, 提供按 “q” 退出
        auto main_logic = CatchEvent(layout, [&, this](Event event) {
            if (event == Event::Character('q')) {
                screen.Exit();
                return true;
            }

            return false;
            // if (event == Event::Character('1')) {
            //     tab_index = 2; // 待完善
            // }
            // return false;
        });

        // 启动一个后台线程，每秒触发一次刷新
        refresh_thread_running = true;
        std::thread refresh_thread([&] {
            while (refresh_thread_running) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1s); // 等待 1 秒

                // 刷新页面
                ctx.request_repaint();
            }
        });

        //  启动主循环
        screen.Loop(main_logic);

        //  退出后清理线程
        refresh_thread_running = false;
        if (refresh_thread.joinable()) {
            refresh_thread.join();
        }
    }

    ~ShopAppUI() {};
};
