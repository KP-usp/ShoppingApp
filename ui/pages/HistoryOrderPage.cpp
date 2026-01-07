#include "HistoryOrderPage.h"
#include "SharedComponent.h"
#include <fstream>

void HistoryOrderLayOut::init_page(
    AppContext &ctx, std::function<void()> on_orders_info,
    std::function<void()> on_shopping,
    std::function<void()> refresh_history_order_page) {

    //  数据加载
    int user_id = (*(ctx.current_user)).id;
    ctx.history_order_manager.load_history_orders(user_id, ctx.product_manager);

    //  底部交互按钮
    auto btn_to_shopping =
        Button("返回商城", on_shopping, ButtonOption::Animated(Color::Green));
    auto btn_to_orders_info =
        Button("订单详情", on_orders_info, ButtonOption::Animated(Color::Cyan));

    // 清除历史订单按钮
    auto btn_clear_history_orders = Button(
        "清除历史订单", [this] { show_popup = 1; },
        ButtonOption::Animated(Color::Red));

    // 弹窗确认按钮
    auto btn_confirm_clear_history_orders = Button(
        "确认清除",
        [this, &ctx, refresh_history_order_page, user_id] {
            ctx.history_order_manager.delete_all_history_orders(user_id);
            show_popup = 0;
            refresh_history_order_page();
        },
        ButtonOption::Animated(Color::Red));

    auto btn_cancel_clear_history_orders = Button(
        "取消",
        [this, refresh_history_order_page] {
            show_popup = 0;
            refresh_history_order_page();
        },
        ButtonOption::Animated(Color::Green));

    // 底部按钮容器
    auto btn_container = Container::Horizontal(
        {btn_to_orders_info, btn_to_shopping, btn_clear_history_orders});

    // 弹窗按钮容器
    auto btn_popup_container = Container::Horizontal(
        {btn_confirm_clear_history_orders, btn_cancel_clear_history_orders});

    // 订单列表逻辑容器
    auto history_order_list_container = Container::Vertical({});

    auto *histroy_orders_map_ptr =
        ctx.history_order_manager.get_history_map_ptr().value_or(nullptr);

    if (histroy_orders_map_ptr && !histroy_orders_map_ptr->empty()) {
        for (auto it = histroy_orders_map_ptr->rbegin();
             it != histroy_orders_map_ptr->rend(); ++it) {
            long long order_id = it->first;

            // 逻辑按钮，用于捕获焦点和滚动
            auto item_logic = Button("", [] {}, ButtonOption::Simple());

            auto item_renderer = Renderer(item_logic, [order_id, this, &ctx,
                                                       item_logic] {
                // 重新获取数据指针
                auto *map_ptr =
                    ctx.history_order_manager.get_history_map_ptr().value_or(
                        nullptr);

                // 如果数据源没了，或者订单ID找不到了，就渲染个空的或错误提示
                if (!map_ptr || map_ptr->find(order_id) == map_ptr->end()) {
                    return text("数据已更新，请刷新") | color(Color::Red);
                }

                HistoryFullOrder &history_full_order = map_ptr->at(order_id);

                Elements content;

                int delivery_idx =
                    history_full_order.items[0].delivery_selection;

                // 检查送达选项是否合规
                if (delivery_idx < 0 || delivery_idx >= 3)
                    return text("数据已更新，请刷新") | color(Color::Red);

                // 预计送达时间
                time_t arrival_t = Utils::add_days_to_time(
                    history_full_order.order_time,
                    delivery_required_time[delivery_idx]);

                auto format_arrival_t =
                    Utils::specific_hour_to_string(arrival_t);

                // 订单状态
                std::string order_status;
                Element status_text;

                if (history_full_order.status == FullOrderStatus::CANCEL) {
                    order_status = "订单已取消";
                    status_text = text(order_status) | color(Color::RedLight);
                }
                if (history_full_order.status == FullOrderStatus::COMPLETED) {
                    order_status = "订单已完成";
                    status_text =
                        text(order_status) | color(Color::GreenLight) | bold;
                }
                // --- 信息头 ---
                content.push_back(
                    hbox({text(" 订单号: " + std::to_string(order_id)),
                          text("    "), text(" 下单时间: "),
                          text(Utils::time_to_string(
                              history_full_order.order_time)) |
                              color(Color::Green),
                          filler(), text("订单状态: "), status_text, filler(),
                          text("预计抵达时间: "),
                          text(format_arrival_t) | color(Color::Green)

                    }));
                content.push_back(separatorLight());

                // --- 商品明细 ---
                for (const auto &item : history_full_order.items) {
                    // 商品信息
                    std::string name = std::string(item.product_name);
                    double price = item.price;

                    content.push_back(hbox({
                        text(" 商品名: "),
                        text(name) | color(Color::Blue) |
                            size(WIDTH, GREATER_THAN, 15),
                        filler(),
                        text("数量: x " + std::to_string(item.count) + " "),
                        filler(),
                        text("￥" + Utils::format_price(price * item.count)) |
                            color(Color::BlueLight),
                    }));
                }

                // --- 配送与总价 ---
                content.push_back(separatorLight());

                // 简化版送达选项
                std::string del_name = delivery_choices[delivery_idx];
                del_name = del_name.substr(0, del_name.find('('));

                double delivery_fee = DELIVERY_PRICES[delivery_idx];

                content.push_back(hbox({
                    text(" 配送方式: "),
                    text(del_name) | color(Color::Cyan),
                    filler(),
                    text("运费: "),
                    text("￥" + Utils::format_price(delivery_fee)) |
                        color(Color::Blue),
                }));

                content.push_back(separatorLight());

                content.push_back(hbox({
                    text(" 送达地址: "),
                    text(history_full_order.address + "") | color(Color::Cyan),
                    filler(),
                    text("合计: ") | bold,
                    text("￥" +
                         Utils::format_price(history_full_order.total_price)) |
                        color(Color::Yellow) | bold,
                }));

                // 样式包装

                bool is_focused = item_logic->Focused();

                Color border_color =
                    is_focused ? Color::Cyan : Color::GrayLight;
                Color bg_color = is_focused
                                     ? static_cast<Color>(Color::Grey23)
                                     : static_cast<Color>(Color::Default);

                // 组装最终卡片样式
                auto card = vbox(content) | borderRounded |
                            color(border_color) | bgcolor(bg_color);
                if (is_focused) {
                    card = card | focus;
                }

                return card;
            });

            history_order_list_container->Add(item_renderer);
        }

        // 构建滚动视图
        auto scroller =
            SharedComponents::Scroller(history_order_list_container);

        auto main_layout = Container::Vertical({
            scroller,
            btn_container,
        });

        auto popup_hint_renderer = Renderer(btn_popup_container, [=] {
            return window(text("清除确认"),
                          vbox({text("确定要所有的历史订单记录吗？") | center,
                                text("取消后无法恢复") | color(Color::Red) |
                                    center,
                                separator(),
                                btn_popup_container->Render() | center})) |
                   size(WIDTH, GREATER_THAN, 40) | clear_under | center;
        });

        // 支持滚轮在按钮和卡片之间的切换
        auto main_view = SharedComponents::allow_scroll_action(main_layout);

        // 弹窗组件
        auto final_container =
            Container::Tab({main_view, popup_hint_renderer}, &show_popup);

        // 最终渲染
        this->component = Renderer(final_container, [=] {
            auto background =
                vbox({// 标题栏
                      vbox({
                          text(" ") | size(HEIGHT, EQUAL, 1),
                          text(" 历 史 订 单") | bold | center,
                          text(" —— 查 看 您 的 消 费 足 迹 —— ") | dim |
                              center | color(Color::GrayLight),
                          text(" ") | size(HEIGHT, EQUAL, 1),
                      }) | borderDouble |
                          color(Color::YellowLight),

                      // 中间滚动区
                      scroller->Render() | flex,

                      // 底部操作区 (使用 filler 实现要求的布局)
                      separatorHeavy(),
                      hbox({
                          filler(),
                          btn_to_orders_info->Render() | size(WIDTH, EQUAL, 15),
                          filler(),
                          btn_to_shopping->Render() | size(WIDTH, EQUAL, 15),
                          filler(),
                          btn_clear_history_orders->Render() |
                              size(WIDTH, EQUAL, 15),
                          filler(),
                      }) | size(HEIGHT, EQUAL, 3)});
            if (show_popup == 1) {
                return dbox({background, final_container->Render()});
            }

            return background;
        });

    } else {

        this->component = Renderer(btn_container, [=] {
            return vbox(
                {// 标题栏
                 vbox({
                     text(" ") | size(HEIGHT, EQUAL, 1),
                     text(" 历 史 订 单") | bold | center,
                     text(" —— 查 看 您 的 消 费 足 迹 —— ") | dim | center |
                         color(Color::GrayLight),
                     text(" ") | size(HEIGHT, EQUAL, 1),
                 }) | borderDouble |
                     color(Color::YellowLight),

                 text("暂无历史订单记录"),

                 separatorHeavy(),
                 hbox({
                     filler(),
                     btn_to_orders_info->Render() | size(WIDTH, EQUAL, 15),
                     filler(),
                     btn_to_shopping->Render() | size(WIDTH, EQUAL, 15),
                     filler(),
                 }) | size(HEIGHT, EQUAL, 3) |
                     center});
        });
    }
}
