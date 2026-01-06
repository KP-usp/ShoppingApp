#include "ShopPage.h"
#include "SharedComponent.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

void ShopLayOut::init_page(AppContext &ctx, std::function<void()> on_checkout,
                           std::function<void()> add_cart) {
    // 初始加载：搜索空字符串获取所有未删除商品
    current_products = ctx.product_manager.search_product("");
    quantities = std::vector<int>(current_products.size(), 0);

    // 定义容器
    auto main_container = Container::Vertical({});              // 存放商品行
    auto scroller = SharedComponents::Scroller(main_container); // 滚动区域

    // [新增] 搜索输入框
    auto search_input = Input(&search_query, "请输入商品名进行搜索...");

    // [新增] 搜索按钮 (触发列表刷新)
    auto btn_search = Button(
        "🔍 搜索",
        [this, &ctx, main_container] {
            // 调用后端搜索接口
            current_products = ctx.product_manager.search_product(search_query);
            // 重置购买数量状态，防止索引错位
            quantities = std::vector<int>(current_products.size(), 0);
            // 重建 UI 列表
            rebuild_product_list_ui(main_container);
        },
        ButtonOption::Animated(Color::Gold1));

    // 跳转购物车页面按钮
    auto btn_to_cart =
        Button("前往购物车", on_checkout, ButtonOption::Animated(Color::Cyan));

    // 添加到购物车 按钮
    auto btn_add = Button(
        "加入购物车",
        [&ctx, this, add_cart] {
            if (all_of(quantities.begin(), quantities.end(),
                       [](int x) { return x == 0; })) {
                show_popup = 1;
                return;
            } else {

                for (int i = 0; i < current_products.size(); i++) {

                    int qty = (quantities)[i];

                    if (qty > 0) {
                        if (current_products[i].stock - qty <= 0) {
                            show_popup = 2;
                            return;
                        }

                        Product &p = (current_products)[i];

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

    // 弹窗确认按钮
    auto hint_popup_btn1 = Button("确定", [this] { show_popup = 0; });
    auto hint_popup_btn2 = Button("确定", [this] { show_popup = 0; });

    // 首次构建列表内容
    rebuild_product_list_ui(main_container);

    // 组装布局
    // 底部按钮栏
    auto btn_container =
        Container::Horizontal({btn_add | flex, btn_to_cart | flex});

    // 页面主体
    auto page_logic = Container::Vertical({
        Container::Horizontal({search_input | flex, btn_search}), // 顶部搜索栏
        scroller | flex,                                          // 中间列表
        btn_container                                             // 底部按钮
    });

    // 支持滚轮
    auto scroll_view = SharedComponents::allow_scroll_action(page_logic);

    // Tab 容器处理弹窗
    auto final_container = Container::Tab(
        {scroll_view, hint_popup_btn1, hint_popup_btn2}, &show_popup);
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

             // 搜索栏
             hbox({text(" 🔎 ") | center,
                   search_input->Render() | borderRounded | flex,
                   btn_search->Render()}) |
                 size(HEIGHT, EQUAL, 3),

             separator(),

             // 列表区
             current_products.empty()
                 ? (vbox({filler(), text("未找到匹配的商品") | center,
                          filler()}) |
                    flex)
                 : (main_container->Render() | vscroll_indicator | frame |
                    flex),

             separator(),

             // 底部按钮
             hbox({filler(), btn_add->Render() | size(WIDTH, EQUAL, 20),
                   filler(), btn_to_cart->Render() | size(WIDTH, EQUAL, 20),
                   filler()}) |
                 size(HEIGHT, EQUAL, 3)});

        // 弹窗处理
        if (show_popup == 1)
            return SharedComponents::popup_with_button_element(
                hint_popup_btn1, background, "您还没有选购任何商品");
        if (show_popup == 2)
            return SharedComponents::popup_with_button_element(
                hint_popup_btn2, background, "当前商品库存不足");

        return background;
    });
}

void ShopLayOut::rebuild_product_list_ui(Component main_container) {
    main_container->DetachAllChildren(); // 清空旧列表

    for (size_t i = 0; i < current_products.size(); ++i) {
        // 数量控制按钮
        auto btn_inc =
            Button("+", [this, i] { quantities[i]++; }, ButtonOption::Ascii());
        auto btn_dec = Button(
            "-",
            [this, i] {
                if (quantities[i] > 0)
                    quantities[i]--;
            },
            ButtonOption::Ascii());

        auto row_layout = Container::Horizontal({btn_dec, btn_inc});

        // 渲染每一行
        auto row_renderer = Renderer(row_layout, [=] {
            const auto &p = current_products[i]; // 获取当前商品
            int qty = quantities[i];
            bool is_focused = row_layout->Focused();

            // 样式定义
            Color border_c = is_focused ? Color::Cyan : Color::GrayDark;
            Color bg_c =
                is_focused ? Color::Grey23 : static_cast<Color>(Color::Default);
            Color qty_c = qty > 0 ? Color::GreenLight : Color::GrayLight;

            // 库存显示逻辑
            Element stock_info;
            if (p.stock <= 0)
                stock_info = text("缺货") | color(Color::White);
            else if (p.stock < 50)
                stock_info = text("仅剩 " + std::to_string(p.stock)) |
                             color(Color::RedLight);
            else
                stock_info = text("库存充足") | color(Color::YellowLight);

            auto left =
                vbox({filler(), text("商品") | color(Color::BlueLight),
                      text(std::to_string(i + 1)) | dim | center, filler()}) |
                size(WIDTH, EQUAL, 6);

            auto right =
                vbox({hbox({text(" " + std::string(p.product_name)) | bold |
                                size(WIDTH, GREATER_THAN, 15),
                            stock_info, filler(),
                            text(Utils::format_price(p.price) + " 元") |
                                color(Color::Gold1) | bold}),
                      separator() | color(Color::GrayDark),
                      hbox({text("购买数量: ") | color(Color::GrayLight),
                            hbox({btn_dec->Render(),
                                  text(" " + std::to_string(qty) + " ") | bold |
                                      color(qty_c) | size(WIDTH, EQUAL, 4),
                                  btn_inc->Render()}) |
                                borderRounded,
                            filler()})}) |
                flex;

            return hbox({left, separator(), right}) | borderRounded |
                   color(border_c) | bgcolor(bg_c);
        });

        main_container->Add(row_renderer);
    }
}
