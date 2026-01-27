#include "ShopPage.h"
#include "SharedComponent.h"
#include <algorithm>
#include <string>
#include <vector>

void ShopLayOut::init_page(AppContext &ctx, std::function<void()> on_checkout,
                           std::function<void()> add_cart) {

    // 初始加载：搜索空字符串获取所有未删除商品
    current_products = ctx.product_manager.search_product("");

    // 存储购物数量容器初始化
    quantities = std::vector<int>(current_products.size(), 0);
    quantities_str = std::vector<std::string>(current_products.size(), "0");

    // 定义容器
    auto main_container = Container::Vertical({});              // 存放商品行
    auto scroller = SharedComponents::Scroller(main_container); // 滚动区域

    // 搜索输入框
    auto search_input = Input(&search_query, "请输入商品名进行搜索...");

    // 允许在按下回车时直接触发搜索
    auto search_input_logic =
        CatchEvent(search_input, [&ctx, this, main_container](Event event) {
            if (event == Event::Return) {
                // 调用后端搜索接口
                current_products =
                    ctx.product_manager.search_product(search_query);
                // 重置购买数量状态，防止索引错位
                quantities = std::vector<int>(current_products.size(), 0);
                // 清空数量输入框内容
                quantities_str =
                    std::vector<std::string>(current_products.size(), "0");
                rebuild_product_list_ui(main_container);
                return true; // 消费事件，不传入 Input，防止换行
            }
            return false;
        });

    // 搜索按钮 (触发列表刷新)
    auto btn_search = Button(
        "🔍 搜索",
        [this, &ctx, main_container] {
            // 调用后端搜索接口
            current_products = ctx.product_manager.search_product(search_query);
            // 重置购买数量状态，防止索引错位
            quantities = std::vector<int>(current_products.size(), 0);
            // 清空数量输入框内容
            quantities_str =
                std::vector<std::string>(current_products.size(), "0");
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

                            quantities[i] = 0;
                            quantities_str[i] = "0";

                            show_popup = 2;
                            return;
                        }

                        Product &p = (current_products)[i];

                        // 添加到购物车数据库中
                        ctx.cart_manager.add_item((*ctx.current_user).id,
                                                  p.product_id, qty);
                        quantities[i] = 0;
                        quantities_str[i] = "0";
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
    auto hint_popup_btn3 = Button("确定", [this] { show_popup = 0; });

    // 首次构建列表内容
    rebuild_product_list_ui(main_container);

    // 组装布局
    // 底部按钮栏
    auto btn_container =
        Container::Horizontal({btn_add | flex, btn_to_cart | flex});

    // 页面主体
    auto page_logic = Container::Vertical({
        Container::Horizontal(
            {search_input_logic | flex, btn_search}), // 顶部搜索栏
        scroller | flex,                              // 中间列表
        btn_container                                 // 底部按钮
    });

    // 支持滚轮
    auto scroll_view = SharedComponents::allow_scroll_action(page_logic);

    // Tab 容器处理弹窗
    auto final_container = Container::Tab(
        {scroll_view, hint_popup_btn1, hint_popup_btn2, hint_popup_btn3},
        &show_popup);
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
                 : (scroller->Render() | flex),

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
        if (show_popup == 3)
            return SharedComponents::popup_with_button_element(
                hint_popup_btn3, background, "输入的商品数量格式错误");

        return background;
    });
}

void ShopLayOut::rebuild_product_list_ui(Component main_container) {
    main_container->DetachAllChildren(); // 清空旧列表

    for (size_t i = 0; i < current_products.size(); ++i) {
        //  定义数量输入框
        // 1. 绑定到 quantities_str[i]
        // 2. 使用 CatchEvent 监听输入，将字符串解析回 int
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
                        // 尝试转换，如果输入了非数字（如 abc），stoi 会抛出异常
                        int val = std::stoi(quantities_str[i]);
                        // 限制负数
                        if (val < 0) {
                            val = 0;
                            quantities_str[i] = "0";
                        }

                        quantities[i] = val;
                    }
                } catch (...) {
                    show_popup = 3; // 数量格式错误提示弹窗
                    quantities[i] = 0;
                    quantities_str[i] = "0";
                }
                return handled;
            });

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
                if (quantities[i] > 0) {
                    quantities[i]--;
                    quantities_str[i] =
                        std::to_string(quantities[i]); // 同步 string
                }
            },
            ButtonOption::Ascii());

        // 行布局
        auto row_layout =
            Container::Horizontal({btn_dec, input_qty_logic, btn_inc});
        // 渲染每一行
        auto row_renderer = Renderer(row_layout, [=] {
            const auto &p = current_products[i]; // 获取当前商品
            int qty = quantities[i];
            bool is_focused = row_layout->Focused();

            // 获焦及样式定义

            // Input 获焦时的状态
            bool input_focused = input_qty->Focused();

            // 整个卡片获焦
            Color border_c = is_focused ? Color::Cyan : Color::GrayDark;
            Color bg_c =
                is_focused ? Color::Grey23 : static_cast<Color>(Color::Default);
            Color qty_c = qty > 0 ? Color::GreenLight : Color::GrayLight;

            // 如果正在输入，高亮文字
            if (input_focused)
                qty_c = Color::White;

            // 库存显示逻辑
            Element stock_info;
            if (p.stock <= 0)
                stock_info = text("缺货") | color(Color::White);
            else if (p.stock < 50)
                stock_info = text("仅剩 " + std::to_string(p.stock) + " 件") |
                             color(Color::RedLight);
            else
                stock_info = text("库存充足") | color(Color::YellowLight);

            auto left =
                vbox({filler(), text("商品") | color(Color::BlueLight),
                      text(std::to_string(i + 1)) | dim | center, filler()}) |
                size(WIDTH, EQUAL, 6);

            auto right =
                vbox({hbox({text(" " + std::string(p.product_name)) | bold |
                                size(WIDTH, GREATER_THAN, 15) |
                                color(Color::BlueLight),
                            filler(), stock_info, filler(),
                            text(Utils::format_price(p.price) + " 元") |
                                color(Color::Gold1) | bold}),
                      separator() | color(Color::GrayDark),
                      hbox({
                          text("购买数量: ") | color(Color::GrayLight),
                          hbox({btn_dec->Render(),
                                // 渲染输入框，限制宽度，居中
                                input_qty->Render() | color(qty_c) | center |
                                    size(WIDTH, EQUAL, 6),
                                btn_inc->Render()}) |
                              borderRounded,
                      })}) |
                flex;

            return hbox({left, separator(), right}) | borderRounded |
                   color(border_c) | bgcolor(bg_c);
        });

        main_container->Add(row_renderer);
    }
}
