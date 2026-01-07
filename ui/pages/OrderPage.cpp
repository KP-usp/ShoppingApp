#include "OrderPage.h"
#include "LocationUtils.h"
#include "SharedComponent.h"
#include "Utils.h"
#include <fstream>
#include <optional>

void OrderLayOut::init_page(AppContext &ctx, std::function<void()> on_checkout,
                            std::function<void()> on_shopping,
                            std::function<void()> on_history_orders_info,
                            std::function<void()> on_orders_update,
                            std::function<void()> on_orders_delete) {

    // 加载数据
    int user_id = (*(ctx.current_user)).id;
    ctx.order_manager.load_full_orders(user_id, ctx.product_manager);

    // 底部导航按钮
    auto btn_to_shopping =
        Button("购物商城", on_shopping, ButtonOption::Animated(Color::Green));
    auto btn_to_cart =
        Button("返回购物车", on_checkout, ButtonOption::Animated(Color::Cyan));
    auto btn_to_history = Button("历史订单", on_history_orders_info,
                                 ButtonOption::Animated(Color::Yellow));

    auto btn_container =
        Container::Horizontal({btn_to_cart, btn_to_shopping, btn_to_history});

    // --- 定义所有弹窗组件 ---
    // [Popup 1] 功能选择弹窗: 选择修改地址还是配送(带样式)
    auto btn_opt_address = Button("修改收货地址", [this] {
        status_text = ""; // 清空之前的提示
        show_popup = 2;   // 跳转到地址输入
    });
    auto btn_opt_delivery = Button("修改配送服务", [this] {
        show_popup = 4; // 跳转到配送选择
    });
    auto btn_opt_cancel = Button("返回", [this] { show_popup = 0; });

    auto popup_select_layout = Container::Vertical(
        {btn_opt_address, btn_opt_delivery, btn_opt_cancel});

    // [Popup 1]  带样式的渲染器
    auto popup_select_renderer = Renderer(popup_select_layout, [=] {
        return window(text("修改订单"),
                      popup_select_layout->Render() | center) |
               size(WIDTH, GREATER_THAN, 40) | size(HEIGHT, GREATER_THAN, 10) |
               clear_under | center;
    });

    // [Popup 2] 地址修改输入组件
    auto input_address_comp = SharedComponents::create_input_with_placeholder(
        &new_address, "请输入新地址...", false);

    auto btn_addr_auto = Button("自动获取", [this, &ctx] {
        status_text = "正在定位...";
        ctx.request_repaint(); // 立即刷新显示"正在定位"

        std::thread([this, &ctx] {
            auto info_opt = LocationUtils::get_current_location();
            if (info_opt.has_value()) {
                auto info = info_opt.value();
                new_address =
                    info.country + " " + info.region + " " + info.city + " ";
                status_text = "定位成功, 请补充门牌号";
            } else {
                status_text = "定位失败, 请检查网络";
            }
            ctx.request_repaint();
        }).detach();
    });

    auto btn_addr_confirm = Button("确认修改", [this, &ctx, on_orders_update] {
        if (new_address.empty() || new_address.length() > 50) {
            status_text = "地址为空或超过50个字符限制";
            return;
        }
        // 调用 OrderManager 更新地址
        ctx.order_manager.update_order_info(temp_selected_order_id, new_address,
                                            -1); // -1 表示不改配送
        // 同样调用 HistoryManager 同步更新
        ctx.history_order_manager.update_history_order_info(
            temp_selected_order_id, new_address, -1);

        status_text = "修改成功";
        show_popup = 3;
    });

    auto btn_addr_back = Button("取消", [this] { show_popup = 0; });

    auto popup_address_layout = Container::Vertical(
        {input_address_comp,
         Container::Horizontal(
             {btn_addr_auto, btn_addr_confirm, btn_addr_back})});

    // [Popup 2]  带样式的渲染器
    auto popup_address_renderer = Renderer(popup_address_layout, [=] {
        return window(text("修改收货地址"),
                      vbox({popup_address_layout->Render(), separator(),
                            text(status_text) | color(Color::Red) | center})) |
               size(WIDTH, GREATER_THAN, 40) | clear_under | center;
    });

    // [Popup 3] 修改地址成功
    auto btn_address_success_ok = Button("确定", [this, on_orders_update] {
        show_popup = 0;
        on_orders_update(); // 关闭弹窗时彻底刷新页面
    });
    auto popup_update_address_renderer_success_layout =
        Container::Vertical({btn_address_success_ok});
    // [Popup 3] 带样式渲染器
    auto popup_update_address_success_renderer =
        Renderer(popup_update_address_renderer_success_layout, [=] {
            return window(
                       text("提示"),
                       vbox({text(status_text) | color(Color::Green) | center,
                             separator(),
                             popup_update_address_renderer_success_layout
                                     ->Render() |
                                 center})) |
                   size(WIDTH, GREATER_THAN, 40) | clear_under | center;
        });

    // [Popup 4] 配送服务选择
    MenuOption menu_opt;
    menu_opt.entries_option.transform = [&](const EntryState &state) {
        return text(state.label) | (state.active ? color(Color::Cyan) | bold
                                                 : color(Color::GrayDark));
    };
    auto menu_delivery =
        Menu(&delivery_choices, &temp_selected_delivery_idx, menu_opt);

    auto btn_delivery_confirm = Button("确认修改", [this, &ctx] {
        // 订单更新配送
        ctx.order_manager.update_order_info(temp_selected_order_id, "",
                                            temp_selected_delivery_idx);
        // 历史订单更新配送
        ctx.history_order_manager.update_history_order_info(
            temp_selected_order_id, "", temp_selected_delivery_idx);

        status_text = "配送方式已更新";
        show_popup = 5;
    });

    auto btn_delivery_back = Button("取消", [this] { show_popup = 0; });

    auto popup_delivery_layout = Container::Vertical(
        {menu_delivery,
         Container::Horizontal({btn_delivery_confirm, btn_delivery_back})});

    // [Popup 4] 带样式渲染器
    auto popup_delivery_renderer = Renderer(popup_delivery_layout, [=] {
        return window(text("选择配送服务"), popup_delivery_layout->Render()) |
               size(WIDTH, GREATER_THAN, 40) | clear_under | center;
    });

    // [Popup 5] 修改送达选项成功
    auto btn_delivery_success_ok = Button("确定", [this, on_orders_update] {
        show_popup = 0;
        on_orders_update(); // 关闭弹窗时彻底刷新页面
    });
    auto popup_update_delivery_renderer_success_layout =
        Container::Vertical({btn_delivery_success_ok});

    // [Popup 5] 带样式渲染器
    auto popup_update_delivery_success_renderer =
        Renderer(popup_update_delivery_renderer_success_layout, [=] {
            return window(
                       text("提示"),
                       vbox({text(status_text) | color(Color::Green) | center,
                             separator(),
                             popup_update_delivery_renderer_success_layout
                                     ->Render() |
                                 center})) |
                   size(WIDTH, GREATER_THAN, 40) | clear_under | center;
        });

    // [Popup 6] 取消订单确认
    auto btn_cancel_yes = Button("确定取消", [this, &ctx, on_orders_delete] {
        // 更新订单状态
        ctx.order_manager.cancel_order(temp_selected_order_id,
                                       ctx.product_manager);
        // 更新历史订单状态
        ctx.history_order_manager.cancel_history_order(temp_selected_order_id,
                                                       ctx.product_manager);

        show_popup = 0;
        on_orders_delete(); // 刷新页面
    });
    auto btn_cancel_no = Button("再想想", [this] { show_popup = 0; });

    auto popup_cancel_layout =
        Container::Horizontal({btn_cancel_yes, btn_cancel_no});

    // [Popup 6] 带样式渲染器
    auto popup_cancel_renderer = Renderer(popup_cancel_layout, [=] {
        return window(text("取消确认"),
                      vbox({text("确定要取消这个订单吗？") | center,
                            text("取消后无法恢复") | color(Color::Red) | center,
                            separator(),
                            popup_cancel_layout->Render() | center})) |
               size(WIDTH, GREATER_THAN, 40) | clear_under | center;
    });

    // 主容器
    auto order_container = Container::Vertical({});

    auto *orders_map_ptr =
        ctx.order_manager.get_orders_map_ptr().value_or(nullptr);

    if (orders_map_ptr && !orders_map_ptr->empty()) {

        for (auto it = orders_map_ptr->rbegin(); it != orders_map_ptr->rend();
             ++it) {
            long long order_id = it->first;
            FullOrder &full_order = it->second;

            // 过滤：只显示未完成的订单
            if (full_order.status != FullOrderStatus::NOT_COMPLETED) {
                continue;
            }

            // 修改按钮组件
            auto btn_modify = Button(
                " ✎ 修改订单 ",
                [this, order_id, &ctx] {
                    auto *map_ptr =
                        ctx.order_manager.get_orders_map_ptr().value_or(
                            nullptr);
                    if (map_ptr && map_ptr->count(order_id)) {
                        FullOrder &current_order = map_ptr->at(order_id);

                        // 安全地复制数据
                        new_address = current_order.address + "";

                        temp_selected_order_id = order_id;
                        int current_sel =
                            current_order.items.empty()
                                ? 0
                                : current_order.items[0].delivery_selection;
                        temp_selected_delivery_idx = current_sel;

                        show_popup = 1; // 打开选择弹窗
                    }
                },
                ButtonOption::Ascii());

            // 删除按钮组件
            auto btn_cancel_order = Button(
                " × 取消订单 ",
                [this, order_id] {
                    temp_selected_order_id = order_id;
                    show_popup = 6; // 打开取消确认弹窗
                },
                ButtonOption::Ascii());

            // 每个订单卡片的组件逻辑
            auto card_controls =
                Container::Horizontal({btn_modify, btn_cancel_order});

            auto item_renderer = Renderer(card_controls, [&ctx, this, order_id,
                                                          btn_modify,
                                                          btn_cancel_order] {
                // 重新获取数据指针
                auto *map_ptr =
                    ctx.order_manager.get_orders_map_ptr().value_or(nullptr);

                // 如果数据源没了，或者订单ID找不到了，就渲染个空的或错误提示
                if (!map_ptr || map_ptr->find(order_id) == map_ptr->end()) {
                    return text("数据已更新，请刷新") | color(Color::Red);
                }

                // 获取当前有效的引用
                FullOrder &full_order = map_ptr->at(order_id);

                // 计算数据
                double total_price = full_order.total_price;
                int del_idx = (!full_order.items.empty())
                                  ? full_order.items[0].delivery_selection
                                  : 0;
                if (del_idx < 0)
                    del_idx = 0;

                std::string del_name = delivery_choices[del_idx];
                del_name = del_name.substr(0, del_name.find('('));

                time_t arrival_t = Utils::add_days_to_time(
                    full_order.order_time, delivery_required_time[del_idx]);

                auto format_arrival_t =
                    Utils::specific_hour_to_string(arrival_t);

                Elements rows;
                // 信息头
                rows.push_back(
                    hbox({text(" 订单号: " + std::to_string(order_id)) | bold,
                          text("    "), text("下单时间: "),
                          text(Utils::time_to_string(full_order.order_time)) |
                              color(Color::Green),
                          filler(), text("预计抵达时间: "),
                          text(format_arrival_t) | color(Color::Green)}));
                rows.push_back(separator());

                // 订单卡片
                for (const auto &item : full_order.items) {
                    auto prod =
                        ctx.product_manager.get_product(item.product_id);
                    std::string p_name =
                        prod.has_value() ? prod->product_name + "" : "未知商品";
                    double p_price = prod.has_value() ? prod->price : 0.0;

                    rows.push_back(hbox(
                        {text(" 商品名: "),
                         text(p_name) | color(Color::Blue) |
                             size(WIDTH, GREATER_THAN, 10) |
                             color(Color::BlueLight),
                         filler(),
                         text("数量: x " + std::to_string(item.count) + " "),
                         filler(),
                         text("￥" +
                              Utils::format_price(p_price * item.count)) |
                             color(Color::BlueLight)}));
                }
                rows.push_back(separator());

                // 地址和运费
                rows.push_back(hbox(
                    {text(" 配送方式: "), text(del_name) | color(Color::Cyan),
                     filler(), text("运费: "),
                     text("￥" +
                          Utils::format_price(DELIVERY_PRICES[del_idx])) |
                         color(Color::Blue)}));
                rows.push_back(hbox({text(" 送达地址: "),
                                     text(std::string(full_order.address)) |
                                         color(Color::Cyan) |
                                         size(WIDTH, GREATER_THAN, 20)}));

                // 底部总价格
                rows.push_back(separator());
                rows.push_back(
                    hbox({filler(), text("合计: ") | bold,
                          text(Utils::format_price(total_price) + " 元") |
                              color(Color::Yellow) | bold}));

                // 按钮区域
                rows.push_back(
                    hbox({filler(), btn_modify->Render() | color(Color::Yellow),
                          text("       "),
                          btn_cancel_order->Render() | color(Color::RedLight),
                          filler()}));

                // 样式包装

                bool is_focused = btn_modify->Focused() || btn_cancel_order;

                Color border_color =
                    is_focused ? Color::Cyan : Color::GrayLight;
                Color bg_color = is_focused
                                     ? static_cast<Color>(Color::Grey23)
                                     : static_cast<Color>(Color::Default);

                return vbox(rows) | borderRounded | color(border_color) |
                       bgcolor(bg_color);
            });

            order_container->Add(item_renderer);
        }

        if (order_container->ChildCount() == 0) {

            // 没有订单的情况下
            order_container->Add(Renderer([] {
                return vbox({text(""), text("目前没有正在进行的订单") | center,
                             text("去商城逛逛吧！") | center}) |
                       color(Color::GrayLight);
            }));

            auto main_logic_container =
                Container::Vertical({order_container, btn_container});

            this->component = Renderer(main_logic_container, [=] {
                return vbox({// 标题栏
                             vbox({
                                 text(" ") | size(HEIGHT, EQUAL, 1),
                                 text(" 当 前 订 单 ") | bold | center,
                                 text(" —— 正 在 快 马 加 鞭 为 您 送 达 —— ") |
                                     dim | center | color(Color::GrayLight),
                                 text(" ") | size(HEIGHT, EQUAL, 1),
                             }) | borderDouble |
                                 color(Color::YellowLight),

                             order_container->Render(),

                             separatorHeavy(),

                             hbox({filler(), btn_to_cart->Render(), filler(),
                                   btn_to_shopping->Render(), filler(),
                                   btn_to_history->Render(), filler()})});
            });

            // 没有订单直接返回，后面的 UI 逻辑不执行
            return;
        }

        auto scroller = SharedComponents::Scroller(order_container);
        auto main_logic_content =
            Container::Vertical({scroller, btn_container});

        // 支持滚轮在按钮和卡片之间的切换
        auto main_view =
            SharedComponents::allow_scroll_action(main_logic_content);

        //  --- 组装主界面逻辑容器 ---
        auto main_logic_container = Container::Tab(
            {
                main_view,              // Index 0: 主界面
                popup_select_renderer,  // Index 1: 选择弹窗 (带样式)
                popup_address_renderer, // Index 2: 地址输入 (带样式)
                popup_update_address_success_renderer, // Index 3: 成功
                                                       // (带样式)
                popup_delivery_renderer,               // Index 4: 配送 (带样式)
                popup_update_delivery_success_renderer, // Index 5:
                                                        // 复用成功弹窗
                popup_cancel_renderer // Index 6: 取消确认 (带样式) },
            },
            &show_popup);

        this->component = Renderer(main_logic_container, [=] {
            auto background =
                vbox({// 标题栏
                      vbox({
                          text(" ") | size(HEIGHT, EQUAL, 1),
                          text(" 当 前 订 单 ") | bold | center,
                          text(" —— 正 在 快 马 加 鞭 为 您 送 达 —— ") | dim |
                              center | color(Color::GrayLight),
                          text(" ") | size(HEIGHT, EQUAL, 1),
                      }) | borderDouble |
                          color(Color::YellowLight),

                      scroller->Render() | flex, separatorHeavy(),

                      hbox({
                          filler(),
                          btn_to_cart->Render() | size(WIDTH, EQUAL, 15),
                          filler(),
                          btn_to_shopping->Render() | size(WIDTH, EQUAL, 15),
                          filler(),
                          btn_to_history->Render() | size(WIDTH, EQUAL, 15),
                          filler(),
                      }) | size(HEIGHT, EQUAL, 3)});

            // 如果没有弹窗，直接返回背景
            if (show_popup == 0) {
                return background;
            }

            //  如果有弹窗
            return dbox({background, main_logic_container->Render()});
        });
    } else {

        // 开始加载时没有订单的情况下
        order_container->Add(Renderer([] {
            return vbox({text(""), text("目前没有正在进行的订单") | center,
                         text("去商城逛逛吧！") | center}) |
                   color(Color::GrayLight);
        }));

        auto main_logic_container =
            Container::Vertical({order_container, btn_container});

        this->component = Renderer(main_logic_container, [=] {
            return vbox({// 标题栏
                         vbox({
                             text(" ") | size(HEIGHT, EQUAL, 1),
                             text(" 当 前 订 单 ") | bold | center,
                             text(" —— 正 在 快 马 加 鞭 为 您 送 达 —— ") |
                                 dim | center | color(Color::GrayLight),
                             text(" ") | size(HEIGHT, EQUAL, 1),
                         }) | borderDouble |
                             color(Color::GreenLight),

                         order_container->Render(),

                         separatorHeavy(),

                         hbox({
                             filler(),
                             btn_to_cart->Render() | size(WIDTH, EQUAL, 15),
                             filler(),
                             btn_to_shopping->Render() | size(WIDTH, EQUAL, 15),
                             filler(),
                             btn_to_history->Render() | size(WIDTH, EQUAL, 15),
                             filler(),
                         }) | size(HEIGHT, EQUAL, 3)});
        });
    }
}
