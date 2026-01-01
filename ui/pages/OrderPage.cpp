#include "OrderPage.h"
#include "SharedComponent.h"
#include <fstream>

void OrderLayOut::init_page(AppContext &ctx, std::function<void()> on_checkout,
                            std::function<void()> on_shopping) {

    // 从数据库获得用户购物车列表
    int user_id = (*(ctx.current_user)).id;
    ctx.order_manager.load_full_orders(user_id, ctx.product_manager);

    // 定义交互按钮组件
    // 跳转购物商城页面按钮
    auto btn_to_shopping = Button("购物商城", [on_shopping] { on_shopping(); });

    // 跳转购物车页面按钮
    auto btn_to_cart = Button("返回购物车", [on_checkout] { on_checkout(); });

    auto btn_container = Container::Horizontal({
        btn_to_cart,
        btn_to_shopping,
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

            // 容纳单个订单的空按钮组件(为了添加滚动属性)
            auto item_logic = Button("", [] {}, ButtonOption::Simple());

            auto item_renderer = Renderer(item_logic, [&full_order, &ctx, this,
                                                       order_id, item_logic] {
                Elements product_rows;

                // 数据准备
                double total_order_price = full_order.total_price;
                int idx = full_order.items[0].delivery_selection;
                if (idx < 0 || idx >= (int)delivery_choices.size())
                    idx = 0;

                time_t arrived_time =
                    Utils::add_days_to_time(full_order.items[0].order_time,
                                            delivery_required_time[idx]);
                std::string format_arrived_time =
                    Utils::specific_hour_to_string(arrived_time);
                double delivery_fee = DELIVERY_PRICES[idx];

                // 顶部信息栏
                product_rows.push_back(hbox({
                    text(" 下单: " +
                         Utils::time_to_string(full_order.order_time)),
                    filler(),
                    text("收货地址: " + full_order.address) |
                        color(Color::Cyan),
                    filler(),
                    text("预计送达: " + format_arrived_time + " ") |
                        color(Color::Green),
                }));
                product_rows.push_back(separator());

                // 商品列表
                for (const auto &item : full_order.items) {
                    auto prod_opt =
                        ctx.product_manager.get_product(item.product_id);
                    if (prod_opt.has_value()) {
                        double item_total = prod_opt->price * item.count;
                        product_rows.push_back(hbox({
                            text(" • " + prod_opt->product_name) |
                                size(WIDTH, GREATER_THAN, 15) |
                                color(Color::Blue),
                            filler(),
                            text("x " + std::to_string(item.count)),
                            filler(),
                            text("￥" + Utils::format_price(item_total)),
                        }));
                    } else {
                        product_rows.push_back(text("商品已失效") |
                                               color(Color::Red));
                    }
                }

                //  费用汇总
                if (!product_rows.empty()) {
                    product_rows.push_back(separator()); // 分割线

                    // 快递费
                    product_rows.push_back(hbox(
                        {text(" • " + delivery_choices[idx]) |
                             color(Color::Cyan) | size(WIDTH, GREATER_THAN, 12),
                         filler(),
                         text("￥" + Utils::format_price(delivery_fee))}));

                    // 总价高亮显示
                    product_rows.push_back(hbox(
                        {filler(), text("总计: ￥" + Utils::format_price(
                                                         total_order_price)) |
                                       bold | color(Color::Yellow)}));
                }

                // 组装卡片
                auto order_card = window(
                    text(" 订单号: " + std::to_string(order_id) + " ") | center,
                    vbox(std::move(product_rows)));

                Element final_element;

                if (item_logic->Focused()) {
                    order_card = order_card | color(Color::Blue) |
                                 bgcolor(Color::Grey11);

                    final_element = vbox({order_card, text(" ")});

                    final_element = final_element | focus;
                } else {
                    final_element = vbox({order_card, text(" ")});
                }

                return final_element;
            });

            order_container->Add(item_renderer);
        }
    } else {
        order_container->Add(
            Renderer([] { return text("暂无订单记录") | center | flex; }));
    }

    auto scroller_container = SharedComponents::Scroller(order_container);
    auto layout = Container::Vertical({scroller_container, btn_container});

    this->component = Renderer(layout, [=] {
        return vbox({text("我的订单") | bold | center | border,
                     scroller_container->Render(), separator(),
                     hbox({
                         filler(),
                         btn_to_cart->Render(),
                         filler(),
                         btn_to_shopping->Render(),
                         filler(),
                     })});
    });
}
