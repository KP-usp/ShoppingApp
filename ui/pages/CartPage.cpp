#include "CartPage.h"
#include <algorithm>
#include <fstream>

void CartLayOut::init_page(AppContext &ctx, std::function<void()> on_shopping,
                           std::function<void()> on_orders_info,
                           std::function<void()> delete_item_success,
                           std::function<void()> checkout_success) {

    // 从数据库获得用户购物车列表
    int user_id = (*(ctx.current_user)).id;
    ctx.cart_manager.load_cart(user_id);
    auto *cart_list_ptr =
        ctx.cart_manager.get_cart_list_ptr().value_or(nullptr);
    if (cart_list_ptr && !cart_list_ptr->empty()) {
        auto &cart_list = *cart_list_ptr;

        // 初始化数据成员
        delivery_selections = std::vector<int>(cart_list.size(), 0);
        is_chosen = std::deque<bool>(cart_list.size(), false);
        quantities = std::vector<int>(cart_list.size(), 0);

        for (int i = 0; i < cart_list.size(); i++) {
            quantities[i] = cart_list[i].count;
        }

        // 定义交互按钮组件
        // 跳转购物商城页面按钮
        auto btn_to_shopping =
            Button("购物商城", [on_shopping] { on_shopping(); });

        // 跳转订单详情页面按钮
        auto btn_to_orders =
            Button("订单详情", [on_orders_info] { on_orders_info(); });

        // 下单按钮
        auto btn_checkout = Button("确认结账", [this] {
            bool all_zero = std::all_of(is_chosen.begin(), is_chosen.end(),
                                        [](bool x) { return x == false; });
            // 判断用户是否选择了商品
            // 结账逻辑迁移到弹窗确认结束
            if (all_zero)
                show_popup = 3;

            else
                show_popup = 1;
        });

        // 按钮组合
        auto btn_container = Container::Horizontal({
            btn_to_shopping,
            btn_checkout,
            btn_to_orders,
        });

        // 弹窗按钮

        // 支付时确定还是取消
        auto btn_payment_yes = Button("确定", [this] { show_popup = 2; });
        auto btn_payment_no = Button("取消", [this] { show_popup = 0; });

        // 提示未选择商品
        auto btn_hint_no_choice = Button("确定", [this] { show_popup = 0; });

        // 提示支付成功
        auto btn_hint_payment_success =
            Button("确定", [&ctx, &cart_list, this, on_shopping, on_orders_info,
                            checkout_success] {
                int user_id = (*(ctx.current_user)).id;

                for (int i = 0; i < cart_list.size(); i++) {
                    if (is_chosen[i] == true) {

                        int product_id = cart_list[i].product_id;
                        int count = quantities[i];
                        int delivery_selected = delivery_selections[i];

                        // 主要是为了加递送选项
                        ctx.cart_manager.update_item(
                            user_id, product_id, count,
                            CartItemStatus::NOT_ORDERED,
                            delivery_selections[i]);
                    }
                }

                // 添加到订单数据库中
                auto ordered_cart_lists = ctx.cart_manager.checkout(user_id);
                ctx.order_manager.add_order(user_id, ordered_cart_lists);

                show_popup = 0;
                checkout_success();
            });

        // 支付方式单选框（水平单选框）
        MenuOption option = MenuOption::Horizontal();
        option.entries_option.transform = [&](const EntryState &state) {
            std::string prefix = state.active ? "(●) " : "(○) ";

            auto res =
                hbox({text(prefix) | color(Color::Cyan), text(state.label)});
            if (state.focused)
                res |= inverted; // 键盘选中时反色
            return res;
        };
        auto payment_menu = Menu(&payment_choices, &payment_method, option);

        auto main_container = Container::Vertical({});

        for (int i = 0; i < cart_list.size(); i++) {

            // 当前商品信息
            int product_id = cart_list[i].product_id;

            auto product_opt = ctx.product_manager.get_product(product_id);
            if (!product_opt.has_value()) {
                break;
            }
            Product product_info =
                ctx.product_manager.get_product(product_id).value();
            std::string product_name = product_info.product_name + "";

            // 自定义勾选框样式
            CheckboxOption check_opt;
            check_opt.transform = [](const EntryState &s) {
                // 选中：亮绿色；未选中：暗灰色
                Color c = s.state ? Color::GreenLight : Color::GrayDark;

                // 聚焦：青色（高亮）
                if (s.focused)
                    c = Color::Cyan;

                // 选中显示对勾，未选中显示空字符
                std::string symbol = s.state ? "✔" : " ";

                // 构建“大方块”
                return text(symbol) | bold | center | size(WIDTH, EQUAL, 3) |
                       size(HEIGHT, EQUAL, 1) | border | color(c);
            };

            // 勾选框组件，商品名放在后面组装
            auto select_checkbox = Checkbox("", &(is_chosen[i]), check_opt);

            // 选择购物车商品购买数量的按钮
            auto btn_increment = Button(
                "+", [this, i] { (quantities)[i]++; }, ButtonOption::Ascii());
            auto btn_decrement = Button(
                "-",
                [this, i] {
                    if ((quantities)[i] > 1)
                        (quantities)[i]--;
                },
                ButtonOption::Ascii());

            // 递送菜单的样式

            MenuOption menu_option = MenuOption::Horizontal();
            menu_option.entries_option.transform =
                [&](const EntryState &state) {
                    return text(state.label) |
                           (state.active ? color(Color::Cyan) | bold
                                         : color(Color::GrayDark));
                };

            // 递送菜单组件
            auto delivery_menu =
                Menu(&delivery_choices, &delivery_selections[i], menu_option);

            // 删除按钮
            auto btn_delete = Button(
                " × 删除 ",
                [this, &ctx, i, &cart_list, on_shopping, on_orders_info,
                 delete_item_success] {
                    CartItem &item = cart_list[i];
                    ctx.cart_manager.delete_item(item.id, item.product_id);
                    delete_item_success();
                },
                ButtonOption::Ascii());

            // 布局逻辑
            auto card_logic_layout = Container::Horizontal(
                {select_checkbox,
                 Container::Vertical(
                     {Container::Horizontal({btn_decrement, btn_increment}),
                      delivery_menu}),
                 btn_delete});

            auto card_renderer = Renderer(card_logic_layout, [=, &ctx] {
                CartItem &p = (*cart_list_ptr)[i];
                int qty = (quantities)[i];
                double unit_price =
                    ctx.product_manager.get_price_by_id(p.product_id);
                double total_item_price =
                    unit_price * qty + DELIVERY_PRICES[delivery_selections[i]];

                // 焦点状态
                bool is_focused = card_logic_layout->Focused();

                // 获取时是否被勾选
                bool selected = is_chosen[i];

                // 颜色策略
                Color border_color =
                    selected
                        ? static_cast<Color>(Color::Green)
                        : (is_focused ? static_cast<Color>(Color::Cyan)
                                      : static_cast<Color>(Color::GrayDark));
                auto bg_color = is_focused
                                    ? static_cast<Color>(Color::Grey23)
                                    : static_cast<Color>(
                                          Color::Default); // 聚焦时背景稍微变亮
                // 左侧的勾选区域
                auto left_part =
                    vbox({
                        filler(),
                        text("选择框") | center | color(Color::Green),
                        select_checkbox->Render() | center,
                        filler(),
                    }) |
                    size(WIDTH, EQUAL, 6) |
                    bgcolor(selected ? static_cast<Color>(Color::Green4)
                                     : static_cast<Color>(Color::Default));

                // 右侧：详细信息
                auto right_part =
                    vbox({// 第一行：商品名 + 价格
                          hbox({text(product_name) | bold |
                                    size(WIDTH, GREATER_THAN, 15) |
                                    color(Color::Blue),
                                filler(),
                                text(Utils::format_price(total_item_price) +
                                     " 元") |
                                    color(Color::Gold1) | bold}),
                          separator() | color(Color::GrayDark), // 弱化的分割线

                          // 第二行：操作区
                          hbox({// 数量控制
                                hbox({text("数量: ") | vcenter,
                                      btn_decrement->Render() | vcenter,
                                      text(" " + std::to_string(qty) + " ") |
                                          bold | center |
                                          size(WIDTH, EQUAL, 4) | vcenter,
                                      btn_increment->Render() | vcenter}) |
                                    borderRounded |
                                    color(Color::GrayLight), // 给数量加个小圆框

                                filler(),

                                // 递送方式
                                vbox({text("配送:") | size(HEIGHT, EQUAL, 1) |
                                          color(Color::GrayLight),
                                      delivery_menu->Render()}),

                                filler(),

                                // 删除按钮 (红色)
                                btn_delete->Render() |
                                    color(Color::RedLight)})}) |
                    flex; // padding

                // 组合卡片
                return hbox({left_part, separator(), right_part | flex}) |
                       borderRounded | color(border_color) | bgcolor(bg_color);
            });

            auto card_renderer_with_event =
                CatchEvent(card_renderer, [&](Event e) {
                    return false; // 事件未被处理，继续传递给内部组件
                });

            main_container->Add(card_renderer_with_event);
        }

        auto content_container =
            Container::Vertical({main_container, btn_container});
        auto btn_payment_container =
            Container::Vertical({btn_payment_yes, btn_payment_no});
        // 弹窗1：支付方式选择
        auto popup_payment_layout =
            Container::Vertical({payment_menu, btn_payment_container});

        // 弹窗2：成功提示
        auto popup_hint_payment_success_layout =
            Container::Vertical({btn_hint_payment_success});

        // 弹窗3：未选择商品提示
        auto popup_hint_no_choice_layout =
            Container::Vertical({btn_hint_no_choice});

        //  关键步骤：给弹窗容器包裹 Renderer 并应用样式
        // 这样让事件系统知道这些组件是被“居中”和“加框”的
        auto popup_payment_renderer = Renderer(popup_payment_layout, [=] {
            return window(text("提示"),
                          vbox({text("请选择支付方式") | center, separator(),
                                payment_menu->Render() | center, separator(),
                                hbox({filler(), btn_payment_yes->Render(),
                                      filler(), btn_payment_no->Render(),
                                      filler()})})) |
                   size(WIDTH, GREATER_THAN, 40) |
                   size(HEIGHT, GREATER_THAN, 8) | clear_under |
                   center; // 在这里应用 center，事件系统才能正确计算坐标
        });

        auto popup_hint_payment_success_renderer =
            Renderer(popup_hint_payment_success_layout, [=] {
                return window(
                           text("提示"),
                           vbox({text("您已成功支付，请按确定返回") | center,
                                 separator(),
                                 // 这里不需要渲染 menu
                                 btn_hint_payment_success->Render() | center |
                                     size(WIDTH, GREATER_THAN, 10)})) |
                       size(WIDTH, GREATER_THAN, 40) |
                       size(HEIGHT, GREATER_THAN, 8) | clear_under | center;
            });

        auto popup_hint_no_choice_renderer =
            Renderer(popup_hint_no_choice_layout, [=] {
                return window(
                           text("提示"),
                           vbox({text("您还未勾选商品") | center, separator(),
                                 // 这里不需要渲染 menu
                                 btn_hint_no_choice->Render() | center |
                                     size(WIDTH, GREATER_THAN, 10)})) |
                       size(WIDTH, GREATER_THAN, 40) |
                       size(HEIGHT, GREATER_THAN, 8) | clear_under | center;
            });

        //  使用 Tab 切换逻辑焦点
        // 将包装好样式的 Renderer 放进 Tab，而不是原始 Container
        auto logic_container = Container::Tab(
            {
                content_container,      // index 0: 正常页面
                popup_payment_renderer, // index 1: 支付弹窗 (带样式)
                popup_hint_payment_success_renderer, // index 2:
                                                     // 提示弹窗(带样式)
                popup_hint_no_choice_renderer,       // index 3:
                                                     // 提示未选择商品（带样式）
            },
            &show_popup);

        //  最终渲染
        this->component = Renderer(logic_container, [=, &cart_list, &ctx] {
            // 计算总价
            double total_price = 0.0;
            for (int i = 0; i < cart_list.size(); i++) {
                int product_id = cart_list[i].product_id;
                total_price += ctx.product_manager.get_price_by_id(product_id) *
                                   quantities[i] +
                               DELIVERY_PRICES[delivery_selections[i]];
            }

            auto background_element = vbox(
                {text("购物车列表") | center, separator(),
                 main_container->Render() | vscroll_indicator | frame | flex,
                 text("总计: " + Utils::format_price(total_price) + " 元") |
                     color(Color::Yellow),
                 hbox({
                     filler(),
                     btn_to_shopping->Render(),
                     filler(),
                     btn_checkout->Render(),
                     filler(),
                     btn_to_orders->Render(),
                     filler(),
                 })});

            // 如果没有弹窗，直接返回背景
            if (show_popup == 0) {
                return background_element;
            }

            return dbox({background_element, logic_container->Render()});
        });
    } else {
        // 定义交互按钮组件
        // 跳转购物商城页面按钮
        auto btn_to_shopping =
            Button("购物商城", [on_shopping] { on_shopping(); });

        // 跳转订单详情页面按钮
        auto btn_to_orders =
            Button("订单详情", [on_orders_info] { on_orders_info(); });

        auto btn_container = Container::Horizontal({
            btn_to_shopping,
            btn_to_orders,
        });

        this->component = Renderer(btn_container, [=] {
            return vbox({text("购物车列表") | center, separator(),
                         text("购物车为空") | center, separator(),
                         text("总计: 0 元"),
                         hbox({
                             filler(),
                             btn_to_shopping->Render(),
                             filler(),
                             btn_to_orders->Render(),
                             filler(),
                         })});
        });
    }
}
