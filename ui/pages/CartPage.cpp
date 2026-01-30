#include "CartPage.h"
#include "LocationUtils.h"
#include "SharedComponent.h"
#include <algorithm>
#include <cctype>

void CartLayOut::init_page(AppContext &ctx, std::function<void()> on_shopping,
                           std::function<void()> on_orders_info,
                           std::function<void()> delete_item_success,
                           std::function<void()> checkout_success) {

    // 从数据库获得用户购物车列表
    int user_id = (*(ctx.current_user)).id;
    ctx.cart_manager.load_cart(user_id);
    auto *cart_list_ptr =
        ctx.cart_manager.get_cart_list_ptr().value_or(nullptr);

    // 购物车列表为空，只渲染界面元素
    if (cart_list_ptr->empty()) {

        // 跳转购物商城页面按钮
        auto btn_to_shopping = Button("购物商城", on_shopping,
                                      ButtonOption::Animated(Color::Green));

        // 跳转订单详情页面按钮
        auto btn_to_orders = Button("订单详情", on_orders_info,
                                    ButtonOption::Animated(Color::Cyan));

        auto btn_container = Container::Horizontal({
            btn_to_shopping,
            btn_to_orders,
        });

        this->component = Renderer(btn_container, [=] {
            return vbox(
                {vbox({
                     text(" ") | size(HEIGHT, EQUAL, 1),
                     text(" 购 物 车 ") | bold | center,
                     text(" —— 心 动 不 如 行 动 —— ") | dim | center |
                         color(Color::GrayLight),
                     text(" ") | size(HEIGHT, EQUAL, 1),
                 }) | borderDouble |
                     color(Color::YellowLight),

                 text("您的购物车目前为空,  请去商品页看看吧！") | center,
                 separator(), text("总计: 0 元") | color(Color::Yellow),
                 hbox({
                     filler(),
                     btn_to_shopping->Render() | size(WIDTH, EQUAL, 15),
                     filler(),
                     btn_to_orders->Render() | size(WIDTH, EQUAL, 15),
                     filler(),
                 }) | size(HEIGHT, EQUAL, 3)});
        });

        return;
    }

    // 列表不为空，获取其引用
    auto &cart_list = *cart_list_ptr;

    // 初始化数据成员
    delivery_selections = std::vector<int>(cart_list.size(), 0);
    is_chosen = std::deque<bool>(cart_list.size(), false);
    quantities = std::vector<int>(cart_list.size(), 0);
    quantities_str = std::vector<std::string>(cart_list.size(), "0");

    for (int i = 0; i < cart_list.size(); i++) {
        quantities[i] = cart_list[i].count;
        quantities_str[i] = std::to_string(cart_list[i].count);
    }

    // 购物车列表容器
    auto main_container = Container::Vertical({});
    // 滚动区域
    auto scroller = SharedComponents::Scroller(main_container);

    // 构建购物车列表
    rebuild_cart_list_ui(main_container, ctx, cart_list, delete_item_success);

    // --- 定义交互按钮组件 ---

    // 跳转购物商城页面按钮
    auto btn_to_shopping =
        Button("购物商城", on_shopping, ButtonOption::Animated(Color::Green));

    // 跳转订单详情页面按钮
    auto btn_to_orders =
        Button("订单详情", on_orders_info, ButtonOption::Animated(Color::Cyan));

    // 下单按钮
    auto btn_checkout = Button(
        "确认结账",
        [this] {
            bool all_zero = std::all_of(is_chosen.begin(), is_chosen.end(),
                                        [](bool x) { return x == false; });
            // 判断用户是否选择了商品
            // 结账逻辑迁移到弹窗确认结束
            if (all_zero)
                show_popup = 3; // 没有选择商品触发提示弹窗

            else
                show_popup = 4; // 触发填写地址弹窗
        },
        ButtonOption::Animated(Color::Yellow));

    // 按钮组合
    auto btn_container = Container::Horizontal({
        btn_to_shopping,
        btn_checkout,
        btn_to_orders,
    });

    // --- 弹窗组件 ---

    // [Popup 1] 支付弹窗组件
    // 支付时确定还是取消
    auto btn_payment_yes = Button("确定", [this] { show_popup = 2; });
    auto btn_payment_no = Button("取消", [this] { show_popup = 0; });

    // 支付方式单选框（水平单选框）
    MenuOption option;
    option.entries_option.transform = [&](const EntryState &state) {
        std::string prefix = state.active ? "(●) " : "(○) ";

        auto res = hbox({text(prefix) | color(Color::Cyan), text(state.label)});
        if (state.focused)
            res |= inverted; // 键盘选中时反色
        return res;
    };
    auto payment_menu = Menu(&payment_choices, &payment_method, option);

    // [Popup 2] 提示支付成功弹窗组件
    auto btn_hint_payment_success = Button("确定", [&ctx, &cart_list, this,
                                                    on_shopping, on_orders_info,
                                                    checkout_success] {
        int user_id = (*(ctx.current_user)).id;

        for (int i = 0; i < cart_list.size(); i++) {
            if (is_chosen[i] == true) {

                int product_id = cart_list[i].product_id;
                int count = quantities[i];
                int delivery_selected = delivery_selections[i];

                // 主要是为了加递送选项
                ctx.cart_manager.update_item(user_id, product_id, count,
                                             delivery_selections[i]);

                auto p_opt = ctx.product_manager.get_product(product_id);
                if (p_opt.has_value()) {
                    auto p = p_opt.value();

                    ctx.product_manager.update_product(
                        p.product_name, p.product_id, p.price, p.stock - count);
                }
            }
        }

        // 添加到订单数据库中
        auto ordered_cart_lists = ctx.cart_manager.checkout(user_id);

        ctx.order_manager.add_order(user_id, ordered_cart_lists, input_address);
        // 添加快照到历史订单数据库
        ctx.history_order_manager.add_history_order(
            user_id, ctx.product_manager, ordered_cart_lists, input_address);

        show_popup = 0;
        checkout_success();
    });

    // [Popup 3] 提示未选择弹窗组件
    auto btn_hint_no_choice = Button("确定", [this] { show_popup = 0; });

    // [Popup 4] 填写收货地址弹窗组件
    // 收货地址输入框
    auto input_address_component =
        SharedComponents::create_input_with_placeholder(
            &input_address, "请输入你的收货地址或点击自动获取", false);

    // 自动选择按钮
    auto btn_get_addr = Button("自动获取", [this, &ctx] {
        status_text = "正在定位中, 请稍后";

        // 启动一个分离线程
        std::thread([&] {
            auto info_opt = LocationUtils::get_current_location();

            if (info_opt.has_value()) {
                auto info = info_opt.value();

                // 将获取的地址存储到类成员中,同时显示在输入框
                input_address =
                    info.country + " " + info.region + " " + info.city + " ";
                status_text = "定位成功！请补充详细门牌号";

            } else
                status_text = "定位失败，请检查网络";

            // 刷新屏幕
            ctx.request_repaint();
        }).detach();
    });

    // 确认地址按钮
    auto btn_assure_addr = Button("确认地址", [this] {
        auto is_space =
            std::all_of(input_address.begin(), input_address.end(),
                        [](unsigned char ch) { return std::isspace(ch); });
        if (input_address.empty() || is_space)
            status_text = "您输入的地址为空，请重新输入";
        else
            show_popup = 1;
    });

    // 取消选择地址按钮
    auto btn_addr_no = Button("取消", [this] { show_popup = 0; });

    // [Popup 5] 提示数量格式错误弹窗组件
    auto btn_qty_format_error = Button("确定", [this] { show_popup = 0; });

    // --- 逻辑容器 ---
    auto popup_payment_layout = Container::Vertical(
        {payment_menu, Container::Vertical({btn_payment_yes, btn_payment_no})});
    auto popup_hint_success_layout =
        Container::Vertical({btn_hint_payment_success});
    auto popup_hint_no_choice_layout =
        Container::Vertical({btn_hint_no_choice});
    auto popup_hint_qty_error_layout =
        Container::Vertical({btn_qty_format_error});

    auto popup_addr_layout = Container::Vertical(
        {input_address_component,
         Container::Horizontal({btn_get_addr, btn_assure_addr, btn_addr_no})});

    // --- 弹窗样式 ---
    // 给弹窗容器包裹 Renderer 并应用样式
    // 避免显示位置与实际位置错位

    // 弹窗的基本样式，窗口标题模板
    auto MakeModal = [&](Component content, std::string title,
                         std::function<Element()> content_renderer) {
        return Renderer(content, [=] {
            return window(text(title), content_renderer()) |
                   size(WIDTH, GREATER_THAN, 40) |
                   size(HEIGHT, GREATER_THAN, 8) | clear_under | center;
        });
    };

    auto popup_payment_renderer = MakeModal(popup_payment_layout, "提示", [=] {
        return vbox({text("请选择支付方式") | center, separator(),
                     payment_menu->Render() | center, separator(),
                     hbox({filler(), btn_payment_yes->Render(), filler(),
                           btn_payment_no->Render(), filler()})});
    });

    auto popup_address_input_renderer =
        MakeModal(popup_addr_layout, "收货地址确认", [=] {
            return vbox({input_address_component->Render() | center |
                             size(WIDTH, GREATER_THAN, 10),
                         text(status_text) | center | color(Color::Red),
                         separator(),
                         hbox({filler(), btn_get_addr->Render(), filler(),
                               btn_assure_addr->Render(), filler(),
                               btn_addr_no->Render(), filler()})});
        });

    // 简单提示弹窗模板
    auto MakeAlert = [&](Component btn, std::string msg, Component container) {
        return MakeModal(container, "提示", [=] {
            return vbox(
                {text(msg) | center, separator(),
                 btn->Render() | center | size(WIDTH, GREATER_THAN, 10)});
        });
    };

    // 简单提示弹窗
    auto popup_hint_payment_success_renderer =
        MakeAlert(btn_hint_payment_success, "您已成功支付，请按确定返回",
                  popup_hint_success_layout);
    auto popup_hint_no_choice_renderer = MakeAlert(
        btn_hint_no_choice, "您还未勾选商品", popup_hint_no_choice_layout);
    auto popup_hint_qty_format_error_renderer = MakeAlert(
        btn_qty_format_error, "商品数量格式错误", popup_hint_qty_error_layout);

    //  使用 Tab 切换逻辑焦点
    // 将包装好样式的 Renderer 放进 Tab，而不是原始 Container
    auto logic_container = Container::Tab(
        {
            Container::Vertical(
                {main_container,
                 btn_container}),                // index 0: 主页面(列表 + 按钮)
            popup_payment_renderer,              // index 1: 支付弹窗 (带样式)
            popup_hint_payment_success_renderer, // index 2:
                                                 // 提示弹窗
            popup_hint_no_choice_renderer,       // index 3:
                                                 // 提示未选择商品
            popup_address_input_renderer,        // index 4:
                                                 // 填写地址弹窗
            popup_hint_qty_format_error_renderer // index 5:
                                                 // 数量格式错误提示弹窗
        },
        &show_popup);

    //  最终渲染
    this->component = Renderer(logic_container, [=, &cart_list, &ctx] {
        // 计算总价
        double total_price = 0.0;
        for (int i = 0; i < cart_list.size(); i++) {
            if (is_chosen[i]) {
                int product_id = cart_list[i].product_id;
                total_price += ctx.product_manager.get_price_by_id(product_id) *
                                   quantities[i] +
                               DELIVERY_PRICES[delivery_selections[i]];
            }
        }

        auto background_element =
            vbox({vbox({
                      text(" ") | size(HEIGHT, EQUAL, 1),
                      text(" 购 物 车 ") | bold | center,
                      text(" —— 心 动 不 如 行 动 —— ") | dim | center |
                          color(Color::GrayLight),
                      text(" ") | size(HEIGHT, EQUAL, 1),
                  }) | borderDouble |
                      color(Color::YellowLight),

                  main_container->Render() | vscroll_indicator | frame | flex,
                  text("总计: " + Utils::format_price(total_price) + " 元") |
                      color(Color::Yellow),
                  hbox({
                      filler(),
                      btn_to_shopping->Render() | size(WIDTH, EQUAL, 15),
                      filler(),
                      btn_checkout->Render() | size(WIDTH, EQUAL, 15),
                      filler(),
                      btn_to_orders->Render() | size(WIDTH, EQUAL, 15),
                      filler(),
                  }) | size(HEIGHT, EQUAL, 3)});

        // 如果没有弹窗，直接返回背景
        if (show_popup == 0) {
            return background_element;
        }

        return dbox({background_element, logic_container->Render()});
    });
}

void CartLayOut::rebuild_cart_list_ui(
    Component main_container, AppContext &ctx, std::vector<CartItem> &cart_list,
    std::function<void()> delete_item_success) {
    main_container->DetachAllChildren();

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
            Color c = s.state ? Color::GreenLight : Color::White;

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

        // 商品数量输入框
        auto input_qty = Input(&quantities_str[i]);

        // 为输入框添加逻辑：当用户输入时，更新 quantities[i] (int)
        auto input_qty_logic =
            CatchEvent(input_qty, [this, i, input_qty](Event event) {
                // 让 Input 组件先处理字符输入
                bool handled = input_qty->OnEvent(event);

                // 数据同步逻辑: String -> Int
                try {
                    if (quantities_str[i].empty()) {
                        quantities[i] = 0;
                    } else {
                        // 尝试转换，如果输入了非数字（如 abc），stoi
                        // 会抛出异常
                        int val = std::stoi(quantities_str[i]);
                        // 限制负数
                        if (val < 0) {
                            val = 0;
                            quantities_str[i] = "1";
                        }

                        quantities[i] = val;
                    }
                } catch (...) {
                    show_popup = 5; // 数量格式错误提示弹窗
                    quantities[i] = 1;
                    quantities_str[i] = "1";
                }
                return handled;
            });

        // 选择购物车商品购买数量的按钮
        // "+" 按钮逻辑：同时更新 int 和 string
        auto btn_inc = Button(
            "+",
            [this, i] {
                quantities[i]++;
                quantities_str[i] =
                    std::to_string(quantities[i]); // 同步 string
            },
            ButtonOption::Ascii());

        // "-" 按钮逻辑：同时更新 int 和 string
        auto btn_dec = Button(
            "-",
            [this, i] {
                if (quantities[i] > 1) {
                    quantities[i]--;
                    quantities_str[i] =
                        std::to_string(quantities[i]); // 同步 string
                }
            },
            ButtonOption::Ascii());

        // 递送菜单的样式

        MenuOption menu_option = MenuOption::Horizontal();
        menu_option.entries_option.transform = [&](const EntryState &state) {
            return text(state.label) | (state.active ? color(Color::Cyan) | bold
                                                     : color(Color::GrayDark));
        };

        // 递送菜单组件
        auto delivery_menu =
            Menu(&delivery_choices, &delivery_selections[i], menu_option);

        // 删除按钮
        auto btn_delete = Button(
            " × 删除 ",
            [this, &ctx, i, &cart_list, delete_item_success] {
                CartItem &item = cart_list[i];
                ctx.cart_manager.delete_item(item.id, item.product_id);
                delete_item_success();
            },
            ButtonOption::Ascii());

        // 布局逻辑
        auto card_logic_layout = Container::Horizontal(
            {select_checkbox,
             Container::Vertical(
                 {Container::Horizontal({btn_dec, input_qty_logic, btn_inc}),
                  delivery_menu}),
             btn_delete});

        auto card_renderer = Renderer(card_logic_layout, [=, &cart_list, &ctx] {
            CartItem &p = (cart_list)[i];
            int qty = (quantities)[i];
            double unit_price =
                ctx.product_manager.get_price_by_id(p.product_id);
            double total_item_price =
                unit_price * qty + DELIVERY_PRICES[delivery_selections[i]];

            // 焦点状态

            // 整个卡片获焦
            bool is_focused = card_logic_layout->Focused();
            // Input 获焦时的状态
            bool input_focused = input_qty->Focused();

            // 获取时是否被勾选
            bool selected = is_chosen[i];

            // 颜色策略
            Color border_color = is_focused
                                     ? static_cast<Color>(Color::Cyan)
                                     : static_cast<Color>(Color::GrayDark);
            auto bg_color =
                is_focused
                    ? static_cast<Color>(Color::Grey23)
                    : static_cast<Color>(Color::Default); // 聚焦时背景稍微变亮
            Color qty_c = qty > 0 ? Color::GreenLight : Color::GrayLight;

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
                vbox(
                    {// 第一行：商品名 + 价格
                     hbox({text(product_name) | bold |
                               size(WIDTH, GREATER_THAN, 15) |
                               color(Color::Blue),
                           filler(),
                           text(Utils::format_price(total_item_price) + " 元") |
                               color(Color::Gold1) | bold}),
                     separator() | color(Color::GrayDark), // 弱化的分割线

                     // 第二行：操作区
                     hbox({// 数量控制
                           hbox({text("数量: ") | vcenter,
                                 btn_dec->Render() | vcenter,
                                 input_qty->Render() | color(qty_c) | center |
                                     size(WIDTH, EQUAL, 6),
                                 btn_inc->Render() | vcenter}) |
                               borderRounded |
                               color(Color::GrayLight), // 给数量加个小圆框

                           filler(),

                           // 递送方式
                           vbox({text("配送:") | size(HEIGHT, EQUAL, 1) |
                                     color(Color::GrayLight),
                                 delivery_menu->Render()}),

                           filler(),

                           // 删除按钮 (红色)
                           btn_delete->Render() | color(Color::RedLight)})}) |
                flex; // padding

            // 组合卡片
            return hbox({left_part, separator(), right_part | flex}) |
                   borderRounded | color(border_color) | bgcolor(bg_color);
        });

        // 为了能够滚轮能在卡片及按钮间滚动
        auto card_renderer_with_event = CatchEvent(card_renderer, [&](Event e) {
            return false; // 事件未被处理，继续传递给内部组件
        });
        main_container->Add(card_renderer_with_event);
    }
}
