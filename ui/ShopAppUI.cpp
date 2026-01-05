#include "ShopAppUI.h"
#include <fstream>

ShopAppUI::ShopAppUI(AppContext &context) : ctx(context) {

    refresh_login_page = [this] {
        if (login_layout)
            login_layout->clear();
    };
    refresh_register_page = [this] {
        if (register_layout)
            register_layout->clear();
    };

    refresh_shop_page = [this] {
        shop_layout->refresh(ctx, on_checkout, add_cart);
        shop_container_slot->DetachAllChildren();
        shop_container_slot->Add(shop_layout->get_component());
    };

    refresh_cart_page = [this] {
        cart_layout->refresh(ctx, on_shopping, on_orders_info,
                             delete_item_success, checkout_success);

        cart_container_slot->DetachAllChildren();
        cart_container_slot->Add(cart_layout->get_component());
    };

    refresh_order_page = [this] {
        order_layout->refresh(ctx, on_checkout, on_shopping,
                              on_history_orders_info, on_orders_update,
                              on_orders_delete);

        order_container_slot->DetachAllChildren();
        order_container_slot->Add(order_layout->get_component());
    };

    refresh_history_order_page = [this] {
        history_order_layout->refresh(ctx, on_orders_info, on_shopping);
        history_order_container_slot->DetachAllChildren();
        history_order_container_slot->Add(
            history_order_layout->get_component());
    };

    on_login = [this] {
        refresh_login_page();
        tab_index = 0; // 跳转到登入页面
    };

    on_logout_success = [this] {
        // 清空登录页（如果有的话）
        refresh_login_page();

        if (ctx.current_user->is_admin) {
            admin_container_slot->DetachAllChildren();
            admin_layout.reset();
        } else {
            // 处理 shoppage, cartpage, orderpage , historyorderpage
            // 将其重新变为空壳
            shop_container_slot->DetachAllChildren();
            shop_layout.reset();
            cart_container_slot->DetachAllChildren();
            cart_layout.reset();
            order_container_slot->DetachAllChildren();
            order_layout.reset();
            history_order_container_slot->DetachAllChildren();
            history_order_layout.reset();
        }
        // 用户登出
        ctx.current_user = nullptr;
        ctx.user_manager.logout();
        tab_index = 0; // 注销成功跳转登入页面
    };

    on_login_success = [this] {
        if (!ctx.current_user)
            return;

        if (ctx.current_user->is_admin) {
            admin_layout = std::make_shared<AdminPortal>(ctx);
            admin_container_slot->DetachAllChildren();
            admin_container_slot->Add(admin_layout->get_component());
            tab_index = 6; // 管理员门户
        } else {
            // 初始化商品页面
            shop_layout =
                std::make_shared<ShopLayOut>(ctx, on_checkout, add_cart);
            shop_container_slot->Add(shop_layout->get_component());

            // 初始化购物车页面

            cart_layout = std::make_shared<CartLayOut>(
                ctx, on_shopping, on_orders_info, delete_item_success,
                checkout_success);

            // 初始化订单页面
            order_layout = std::make_shared<OrderLayOut>(
                ctx, on_checkout, on_shopping, on_history_orders_info,
                on_orders_update, on_orders_delete);

            // 初始化历史订单页面
            history_order_layout = std::make_shared<HistoryOrderLayOut>(
                ctx, on_orders_info, on_shopping);

            // 加载用户内容
            cart_container_slot->DetachAllChildren();
            order_container_slot->DetachAllChildren();
            history_order_container_slot->DetachAllChildren();

            cart_container_slot->Add(cart_layout->get_component());
            order_container_slot->Add(order_layout->get_component());
            history_order_container_slot->Add(
                history_order_layout->get_component());

            tab_index = 2; // 普通用户商品页
        }
    };

    on_register = [this] {
        refresh_register_page();
        tab_index = 1; // 跳转到注册页面
    };

    on_register_success = [this] {
        refresh_login_page();
        tab_index = 0; // 注册成功跳转会登录页面
    };

    on_checkout = [this] {
        tab_index = 3; // 跳转购物车页面
    };

    on_shopping = [this] {
        tab_index = 2; // 跳转到商城页面
    };

    add_cart = [this] { refresh_cart_page(); };

    on_orders_info = [this] {
        tab_index = 4; // 跳转到订单详情页面
    };

    delete_item_success = [this] { refresh_cart_page(); };

    checkout_success = [this] {
        refresh_shop_page();
        refresh_cart_page();
        refresh_order_page();
    };

    on_orders_update = [this] { refresh_order_page(); };

    on_orders_delete = [this] {
        refresh_shop_page();
        refresh_order_page();
        refresh_history_order_page();
    };

    on_history_orders_info = [this] { tab_index = 5; };
}
