#include "ShopPage.h"
#include "SharedComponent.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

void ShopLayOut::init_page(AppContext &ctx, std::function<void()> on_checkout,
                           std::function<void()> add_cart) {
    // 从数据库获得商品列表
    ctx.product_manager.load_product();
    auto *product_list_ptr =
        ctx.product_manager.get_product_list_ptr().value_or(nullptr);

    if (product_list_ptr && !product_list_ptr->empty()) {

        auto &product_list = *product_list_ptr;

        // 商品的总数量
        int count = product_list.size();

        // 初始化每个商品的购买数量状态
        quantities = std::vector<int>(count, 0);

        // 创建主容器（垂直布局）, 用来放所有的商品行
        auto main_container = Container::Vertical({});

        // 跳转购物车页面按钮
        auto btn_to_cart = Button("前往购物车", on_checkout,
                                  ButtonOption::Animated(Color::Cyan));

        // 添加到购物车 按钮
        auto btn_add = Button(
            "加入购物车",
            [&ctx, this, &product_list, add_cart] {
                if (all_of(quantities.begin(), quantities.end(),
                           [](int x) { return x == 0; })) {
                    show_popup = 1;
                    return;
                } else {

                    for (int i = 0; i < product_list.size(); i++) {

                        int qty = (quantities)[i];

                        if (qty > 0) {
                            if (product_list[i].stock - qty <= 0) {
                                show_popup = 2;
                                return;
                            }

                            Product &p = (product_list)[i];

                            // 添加到购物车数据库中
                            ctx.cart_manager.add_item((*ctx.current_user).id,
                                                      p.product_id, qty);
                            quantities[i] = 0;
                        }
                    }
                    // 刷新购物车页面
                    add_cart();
                }
            },
            ButtonOption::Animated(Color::Green));

        // 弹窗组件
        // 用户没有选择任何商品时点击加入购物车
        auto hint_popup_for_qty = Button("确定", [this] { show_popup = 0; });
        // 用户在库存为 0 时点击加入购物车
        auto hint_popup_for_stock = Button("确定", [this] { show_popup = 0; });

        // 循环生成每一行的组件
        for (int i = 0; i < count; ++i) {

            // 选择商品购买数量的按钮
            auto btn_increment = Button(
                "+", [this, i] { (quantities)[i]++; }, ButtonOption::Ascii());
            auto btn_decrement = Button(
                "-",
                [this, i] {
                    if ((quantities)[i] > 0)
                        (quantities)[i]--;
                },
                ButtonOption::Ascii());

            auto row_layout =
                Container::Horizontal({btn_decrement, btn_increment});

            auto row_renderer = Renderer(row_layout, [=, &product_list] {
                Product &p = (product_list)[i];
                int qty = (quantities)[i];

                // 判断是否有购买数量，改变高亮状态
                bool has_qty = qty > 0;

                // 获取焦点状态
                bool is_focused = row_layout->Focused();

                //  颜色策略
                Color border_color = is_focused ? Color::Cyan : Color::GrayDark;
                Color bg_color = is_focused
                                     ? static_cast<Color>(Color::Grey23)
                                     : static_cast<Color>(Color::Default);
                Color qty_text_color =
                    has_qty ? Color::GreenLight : Color::GrayLight;

                // [UI 修改] 构建左侧部分：装饰条或图标
                auto left_part =
                    vbox({filler(),
                          text("商品") | center | color(Color::BlueLight),
                          text(std::to_string(i + 1)) | center | dim,
                          filler()}) |
                    size(WIDTH, EQUAL, 6);

                Element stock_info;
                if (p.stock < 100) {
                    stock_info =
                        text("仅剩 " + std::to_string(p.stock) + " 件") |
                        color(Color::RedLight);
                } else {
                    stock_info = text("库存充足") | color(Color::White);
                }
                // 构建右侧部分：详情信息
                auto right_part =
                    vbox(
                        {// 第一行：商品名称 和 价格
                         hbox({text(" " + p.product_name) | bold |
                                   size(WIDTH, GREATER_THAN, 15) |
                                   color(Color::White),
                               stock_info, filler(),
                               text(Utils::format_price(p.price) + " 元") |
                                   color(Color::Gold1) | bold}),

                         separator() | color(Color::GrayDark), // 弱化的分割线

                         // 第二行：操作区（数量控制）
                         hbox({text("购买数量: ") | vcenter |
                                   color(Color::GrayLight),

                               hbox({btn_decrement->Render() | vcenter,
                                     text(" " + std::to_string(qty) + " ") |
                                         bold | center | size(WIDTH, EQUAL, 4) |
                                         vcenter | color(qty_text_color),
                                     btn_increment->Render() | vcenter}) |
                                   borderRounded | color(Color::GrayLight),

                               filler()})}) |
                    flex; // padding

                // 组合卡片
                return hbox({left_part, separator(), right_part}) |
                       borderRounded | color(border_color) | bgcolor(bg_color);
            });
            main_container->Add(row_renderer);
        }

        auto btn_container =
            Container::Horizontal({btn_add | flex, btn_to_cart | flex});

        auto scroller = SharedComponents::Scroller(main_container);

        auto page_container =
            Container::Vertical({main_container, btn_container});

        auto scroll_page_container =
            SharedComponents::allow_scroll_action(page_container);

        auto final_container = Container::Tab(
            {scroll_page_container, hint_popup_for_qty, hint_popup_for_stock},
            (int *)&show_popup);

        // 最终返回一个可滚动的页面
        this->component = Renderer(final_container, [=] {
            auto background = vbox(
                {// 标题栏
                 vbox({
                     text(" ") | size(HEIGHT, EQUAL, 1),
                     text(" 商   品   列   表 ") | bold | center |
                         color(Color::Cyan),
                     text(" —— 物 美 价 廉  任 君 挑 选  —— ") | dim | center |
                         color(Color::GrayLight),
                     text(" ") | size(HEIGHT, EQUAL, 1),

                 }) | borderDouble |
                     color(Color::Cyan),
                 // 列表内容区 (带滚动条)
                 main_container->Render() | vscroll_indicator | frame | flex,

                 separator(),

                 // 底部操作区
                 hbox({
                     filler(),
                     btn_add->Render() |
                         size(WIDTH, EQUAL, 20), // 固定宽度更好看
                     filler(),
                     btn_to_cart->Render() | size(WIDTH, EQUAL, 20),
                     filler(),
                 }) | size(HEIGHT, EQUAL, 3)});

            if (show_popup == 1) {
                return SharedComponents::popup_with_button_element(
                    hint_popup_for_qty, background, "您还没有选购任何商品");
            }

            if (show_popup == 2) {
                return SharedComponents::popup_with_button_element(
                    hint_popup_for_stock, background,
                    "当前商品库存不足，请联系管理员进行添加");
            }
            return background;
        });
    } else {
        this->component = Renderer([] {
            return vbox({vbox({
                             text(" 商 品 列 表 ") | bold | center,
                         }) | borderDouble |
                             color(Color::CyanLight),

                         filler(), text("暂无商品信息") | center | bold,
                         text("请联系管理员添加商品") | center | dim,
                         filler()});
        });
    }
}
