#pragma once
#include "AdminPortal.h"
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
                       // 2-商城 3-购物车 4-当前订单 5-历史订单 6-管理员门户

    // 页面类成员， 统一管理
    std::shared_ptr<LoginLayOut> login_layout;
    std::shared_ptr<RegisterLayOut> register_layout;
    std::shared_ptr<ShopLayOut> shop_layout;
    std::shared_ptr<CartLayOut> cart_layout;
    std::shared_ptr<OrderLayOut> order_layout;
    std::shared_ptr<HistoryOrderLayOut> history_order_layout;
    std::shared_ptr<AdminPortal> admin_layout;

    // 定义“外壳”
    Component shop_container_slot = Container::Vertical({});

    // 定义“外壳”, 用来放购物车页面,订单页面, 或历史订单页面
    // UI(需要登录后初始化)
    Component cart_container_slot = Container::Vertical({});
    Component order_container_slot = Container::Vertical({});
    Component history_order_container_slot = Container::Vertical({});
    Component admin_container_slot = Container::Vertical({});

    Component main_container; // 整个 App 的主容器
    Component tab_container;  // 页面切换容器

    // 原子布尔值，用来控制后台线程何时停止
    std::atomic<bool> refresh_thread_running = false;

    // 全部的 lambda 回调函数

    // 底层具体刷新页面的回调函数
    std::function<void()> refresh_login_page;    // 刷新登录页
    std::function<void()> refresh_register_page; // 刷新注册页
    std::function<void()> refresh_shop_page;     // 刷新商品页（主要是库存）
    std::function<void()> refresh_cart_page;     // 刷新购物车页
    std::function<void()> refresh_order_page;    // 刷新订单页
    std::function<void()> refresh_history_order_page; // 刷新历史订单页

    // 页面逻辑相关的回调函数（使用底层回调、页面 Index 切换）
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
    std::function<void()> checkout_success; // 结账刷新  shop_page(更新库存)、
                                            // cart_page 和 order_page
    std::function<void()> on_history_orders_info; // 前往历史订单页
    std::function<void()> on_orders_update;       // 修改订单刷新 orderpage
    std::function<void()> on_orders_delete; //  删除订单刷新 shop_page(更新库存)
                                            //  、order_page 和 history_page
  public:
    explicit ShopAppUI(AppContext &context);

    AppContext &get_context() { return ctx; }

    // UI 界面运行主逻辑
    void run() {
        auto screen = ScreenInteractive::Fullscreen();

        // 任何持有 ctx 的页面都可以通过调用 ctx.request_repaint() 来刷新屏幕
        ctx.request_repaint = [&screen] { screen.Post(Event::Custom); };

        // 创建页面实例(shop_layout、 cart_layout、order_layout、
        // history_order_layout 都需要登录后创建)
        login_layout =
            std::make_shared<LoginLayOut>(ctx, on_login_success, on_register);
        register_layout = std::make_shared<RegisterLayOut>(
            ctx, on_register_success, on_login);

        // 使用 Tab 容器进行路由管理
        auto tab_content = Container::Tab(
            {login_layout->get_component(), register_layout->get_component(),
             shop_container_slot, cart_container_slot, order_container_slot,
             history_order_container_slot, admin_container_slot},
            &tab_index);

        // 注销组件(放在导航栏中)
        auto btn_logout = Button("注销", [this] { on_logout_success(); });

        // 确保当前有用户时才激活注销组件
        auto logout_logic = Maybe(Container::Vertical({btn_logout}), [this] {
            return ctx.current_user != nullptr;
        });

        auto final_content = Container::Vertical({tab_content, logout_logic});

        //  全局导航栏 (只有登录后才显示)
        auto layout = Renderer(final_content, [&] {
            Element page = tab_content->Render();

            if (ctx.current_user == nullptr) {
                return page;
            }

            // 通用元素
            auto clock_element =
                SharedComponents::get_clock_element() | color(Color::White);
            auto user_element = text(ctx.current_user->username + "") | bold |
                                color(Color::Gold1);
            // 分支渲染
            Element header;

            if (ctx.current_user->is_admin) {
                // 管理员导航栏
                auto admin_header_content = hbox(
                    {text("  "),
                     hbox({text("🛡️ ") | size(WIDTH, EQUAL, 2),
                           text("系统管理后台") | bold |
                               color(Color::RedLight)}) |
                         vcenter | flex,

                     hbox({filler(), text("🕒 "), clock_element, filler()}) |
                         vcenter | flex,

                     hbox({
                         filler(), text("Admin: ") | vcenter | dim,
                         user_element | vcenter, text("  "), separator(),
                         text("  "),
                         logout_logic->Render() // 注销按钮
                     }) | flex,
                     text("  ")});

                header = vbox({
                             admin_header_content,
                             separator() |
                                 color(Color::Red) // 管理员用红色分割线区分
                         }) |
                         bgcolor(Color::Grey11);
            } else {

                // 普通用户导航栏
                auto header_content = hbox(
                    {text("  "),

                     hbox({text("🛒 ") | size(WIDTH, EQUAL, 2),
                           text("购物商城") | bold | color(Color::CyanLight)}) |
                         vcenter | flex,

                     hbox({filler(), text("🕒 ") | vcenter,
                           clock_element | vcenter, filler()}) |
                         flex,

                     hbox({filler(),
                           text("Hi, ") | dim | color(Color::GrayLight) |
                               vcenter,
                           user_element | vcenter,
                           text("  ") | size(WIDTH, EQUAL, 2),
                           separator() | color(Color::GrayDark),
                           text("  ") | size(WIDTH, EQUAL, 1),
                           logout_logic->Render()}) |
                         flex,
                     text("  ")});

                header = vbox({header_content,
                               separator() | color(Color::GrayDark)}) |
                         bgcolor(Color::Grey11);
            }
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
