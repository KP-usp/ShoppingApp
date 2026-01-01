#pragma once
#include "AppContext.h"
#include "CartPage.h"
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

    // 定义“外壳”，为了刷新商品页面
    Component shop_container_slot = Container::Vertical({});

    // 定义“外壳”, 专门用来放购物车页面和订单页面(需要登录后才能启用)
    Component cart_container_slot = Container::Vertical({});
    Component order_container_slot = Container::Vertical({});

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

  public:
    explicit ShopAppUI(AppContext &context);

    AppContext &get_context() { return ctx; }

    void run() {
        auto screen = ScreenInteractive::Fullscreen();

        // 任何持有 ctx 的页面都可以通过调用 ctx.request_repaint() 来刷新屏幕
        ctx.request_repaint = [&screen] { screen.Post(Event::Custom); };

        // 创建页面实例(cart_layout 和 order_layout 都需要登录后创建)
        login_layout =
            std::make_shared<LoginLayOut>(ctx, on_login_success, on_register);
        register_layout = std::make_shared<RegisterLayOut>(
            ctx, on_register_success, on_login);
        shop_layout = std::make_shared<ShopLayOut>(ctx, on_checkout, add_cart);

        // 登录前先初始化 sho_page, cart_page 和 order_page 外壳(其实可以为空)
        cart_container_slot->Add(
            Renderer([] { return text("请先登陆以查看购物车") | center; }));

        order_container_slot->Add(
            Renderer([] { return text("请先登陆以查看订单页面") | center; }));
        shop_container_slot->Add(shop_layout->get_component());

        // 使用 Tab 容器进行路由管理
        // 当 tab_index 变化时，显示的组件也会变化
        // 这里 cart_component 开始时是 “空壳”
        auto tab_content = Container::Tab(
            {login_layout->get_component(), register_layout->get_component(),
             shop_container_slot, cart_container_slot, order_container_slot},
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

            // 如果未登录，只显示页面内容（即登录页）
            if (ctx.current_user == nullptr) {
                return page;
            }

            // 如果已登录，显示顶部导航栏 + 页面内容
            return vbox(
                {hbox({// 给文本加上 |
                       // vcenter，让它相对于这一行最高的元素（按钮）居中
                       text(" 购物软件 ") | bold | vcenter, filler(),
                       SharedComponents::get_clock_element() |
                           vcenter, // 时钟也可能需要居中
                       filler(),
                       text("用户: " + ctx.current_user->username) |
                           vcenter, // 用户名居中
                       filler(), logout_logic->Render()}) |
                     inverted | size(HEIGHT, EQUAL, 3),
                 page | flex});
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
