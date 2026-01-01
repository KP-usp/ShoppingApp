#include "ShopAppUI.h"
#include <fstream>

ShopAppUI::ShopAppUI(AppContext &context) : ctx(context) {
    on_login = [&, this] {
        if (login_layout)
            login_layout->clear();
        tab_index = 0; // 跳转到登入页面
    };

    on_logout_success = [&, this] {
        // 清空登录页及购物页的操作（如果有的话）
        if (login_layout)
            login_layout->clear();

        // 刷新商品页
        shop_layout->refresh(ctx, on_checkout, add_cart);
        shop_container_slot->DetachAllChildren();
        shop_container_slot->Add(shop_layout->get_component());

        // 处理 cartpage, orderpage , 将其重新变为空壳
        cart_container_slot->DetachAllChildren();
        cart_layout.reset();
        order_container_slot->DetachAllChildren();
        order_layout.reset();

        // 用户登出
        ctx.current_user = nullptr;
        ctx.user_manager.logout();
        tab_index = 0; // 注销成功跳转登入页面
    };

    on_login_success = [&, this] {
        // 初始化购物车页面

        cart_layout =
            std::make_shared<CartLayOut>(ctx, on_shopping, on_orders_info,
                                         delete_item_success, checkout_success);

        // 初始化订单页面
        order_layout =
            std::make_shared<OrderLayOut>(ctx, on_checkout, on_shopping);

        // 加载用户内容
        cart_container_slot->DetachAllChildren();
        order_container_slot->DetachAllChildren();
        cart_container_slot->Add(cart_layout->get_component());
        order_container_slot->Add(order_layout->get_component());

        tab_index = 2; // 跳转到商城页面
    };

    on_register = [&, this] {
        if (register_layout)
            register_layout->clear();
        tab_index = 1; // 跳转到注册页面
    };

    on_register_success = [&, this] {
        if (login_layout)
            login_layout->clear();
        tab_index = 0; // 注册成功跳转会登录页面
    };

    on_checkout = [&, this] {
        tab_index = 3; // 跳转购物车页面
    };

    on_shopping = [&, this] {
        tab_index = 2; // 跳转到商城页面
    };

    add_cart = [&, this] {
        cart_layout->refresh(ctx, on_shopping, on_orders_info,
                             delete_item_success, checkout_success);

        cart_container_slot->DetachAllChildren();
        cart_container_slot->Add(cart_layout->get_component());
    };

    on_orders_info = [&, this] {
        tab_index = 4; // 跳转到订单详情页面
    };

    delete_item_success = [&, this] {
        cart_layout->refresh(ctx, on_shopping, on_orders_info,
                             delete_item_success, checkout_success);

        cart_container_slot->DetachAllChildren();
        cart_container_slot->Add(cart_layout->get_component());
    };

    checkout_success = [&, this] {
        cart_layout->refresh(ctx, on_shopping, on_orders_info,
                             delete_item_success, checkout_success);

        cart_container_slot->DetachAllChildren();
        cart_container_slot->Add(cart_layout->get_component());

        order_layout->refresh(ctx, on_checkout, on_shopping);

        order_container_slot->DetachAllChildren();
        order_container_slot->Add(order_layout->get_component());

        std::string path2 = "data/debug.log";

        std::ofstream outfile2(path2, std::ios_base::app);
        if (outfile2.is_open()) {

            outfile2 << "Order 已经成功刷新了 " << std::endl;
            outfile2.close();
        }
    };
}
