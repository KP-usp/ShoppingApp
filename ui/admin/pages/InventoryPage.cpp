#include "InventoryPage.h"
#include "SharedComponent.h"
#include <vector>

void InventoryLayOut::init_page(AppContext &ctx,
                                std::function<void()> back_dashboard,
                                std::function<void()> refresh_inventory_page) {
    // 加载商品数据
    ctx.product_manager.load_product();

    //  定义UI组件容器
    auto list_container = Container::Vertical({});

    //  搜索组件
    // Input 每次输入都会更新 search_query
    auto search_input = Input(&search_query, "输入 ID 或 商品名称进行搜索...");

    // 搜索按钮 (手动触发刷新)
    auto btn_search = Button(
        "🔍 搜索",
        [this, &ctx, list_container, back_dashboard] {
            refresh_list(ctx, list_container, back_dashboard);
        },
        ButtonOption::Animated(Color::Gold1));

    // 返回 Dashboard 按钮
    auto btn_back = Button("返回控制台", back_dashboard,
                           ButtonOption::Animated(Color::RedLight));

    // 定义弹窗组件 (编辑模式)
    auto input_edit_name = Input(&edit_name, "商品名称");
    auto input_edit_price = Input(&edit_price_str, "价格");
    auto input_edit_stock = Input(&edit_stock_str, "库存数量");

    auto btn_save_edit = Button(
        "保存修改",
        [this, &ctx, list_container, back_dashboard, refresh_inventory_page] {
            try {
                double price = std::stod(edit_price_str);
                int stock = std::stoi(edit_stock_str);

                if (edit_name.empty()) {
                    status_msg = "商品名不能为空";
                    return;
                }

                // 调用 Manager 更新
                auto res = ctx.product_manager.update_product(
                    edit_name, selected_product_id, price, stock);

                if (res == FileErrorCode::OK) {
                    show_popup = 0;
                    refresh_list(ctx, list_container,
                                 back_dashboard); // 刷新列表
                    refresh_inventory_page();     // 刷新页面
                } else {
                    status_msg = "更新失败，数据库错误";
                }
            } catch (...) {
                status_msg = "价格或库存格式错误！";
            }
        },
        ButtonOption::Animated(Color::Green));

    auto btn_cancel_edit = Button("取消", [this] { show_popup = 0; });

    auto edit_container = Container::Vertical(
        {input_edit_name, input_edit_price, input_edit_stock,
         Container::Horizontal({btn_save_edit, btn_cancel_edit})});

    // 定义弹窗组件 (删除确认)
    auto btn_confirm_del = Button(
        "确认删除",
        [this, &ctx, list_container, back_dashboard, refresh_inventory_page] {
            ctx.product_manager.delete_product(selected_product_id);
            show_popup = 0;
            refresh_list(ctx, list_container, back_dashboard);
            refresh_inventory_page(); // 刷新页面
        },
        ButtonOption::Animated(Color::Red));

    auto btn_cancel_del = Button("手滑了", [this] { show_popup = 0; });

    auto del_container =
        Container::Horizontal({btn_confirm_del, btn_cancel_del});

    // --- 初始加载列表 ---
    refresh_list(ctx, list_container, back_dashboard);

    // --- 组装主页面结构 ---
    auto main_layout = Container::Vertical({
        Container::Horizontal({search_input, btn_search}), // 顶部工具栏
        list_container,                                    // 中间列表
        btn_back                                           // 底部按钮
    });

    // 支持鼠标滚动进度条
    auto scroller = SharedComponents::Scroller(list_container);
    auto main_logic_content = Container::Vertical({scroller, btn_back});
    auto main_view = SharedComponents::allow_scroll_action(main_logic_content);

    auto final_main_layout = Container::Vertical(
        {Container::Horizontal({search_input | flex, btn_search}), main_view});

    // --- Tab 容器管理页面和弹窗 ---
    auto tab_container = Container::Tab(
        {
            final_main_layout, // 0
            edit_container,    // 1
            del_container      // 2
        },
        &show_popup);

    // --- 最终渲染器 ---
    this->component = Renderer(tab_container, [=] {
        // 背景层 (列表页)
        auto background =
            vbox({vbox({
                      text(" ") | size(HEIGHT, EQUAL, 1),
                      text(" 商   品   库   存   管   理 ") | bold | center |
                          color(Color::Red),
                      text(" —— INVENTORY MANAGEMENT SYSTEM —— ") | dim |
                          center | color(Color::GrayLight),
                      text(" ") | size(HEIGHT, EQUAL, 1),
                  }) | borderDouble |
                      color(Color::Red),

                  // 搜索栏区域
                  hbox({text("🔍 ") | center,
                        search_input->Render() | borderRounded | flex,
                        btn_search->Render()}),

                  separator(),

                  // 列表区域
                  scroller->Render() | flex,

                  separator(),

                  // 底部按钮
                  hbox({
                      filler(),
                      btn_back->Render() | center | size(HEIGHT, EQUAL, 3),
                      filler(),
                  })});

        if (show_popup == 1) {
            // 编辑弹窗渲染
            return dbox(
                {background,
                 window(text(" 修改商品信息 "),
                        vbox({hbox({text("名称: "),
                                    input_edit_name->Render() | flex}),
                              text(" "),
                              hbox({text("价格: "),
                                    input_edit_price->Render() | flex}),
                              text(" "),
                              hbox({text("库存: "),
                                    input_edit_stock->Render() | flex}),
                              separator(),
                              text(status_msg) | color(Color::Red) |
                                  center, // 错误提示
                              separator(),
                              hbox({btn_save_edit->Render() | flex,
                                    btn_cancel_edit->Render() | flex})})) |
                     size(WIDTH, GREATER_THAN, 50) | center | clear_under});
        } else if (show_popup == 2) {
            // 删除弹窗渲染
            return dbox(
                {background,
                 window(
                     text(" 危险操作警告 "),
                     vbox({text("确定要删除 ID: " +
                                std::to_string(selected_product_id) + " 吗？") |
                               center,
                           text("此操作将下架该商品，且不可恢复！") |
                               color(Color::Red) | center,
                           separator(), del_container->Render() | center})) |
                     size(WIDTH, GREATER_THAN, 40) | center | clear_under});
        }

        return background;
    });
}

void InventoryLayOut::refresh_list(AppContext &ctx, Component list_container,
                                   std::function<void()> on_back) {
    list_container->DetachAllChildren();

    //  获取搜索结果
    auto products = ctx.product_manager.search_products(search_query);

    if (products.empty()) {
        list_container->Add(Renderer([] {
            return text("没有找到符合条件的商品") | center | dim |
                   size(HEIGHT, GREATER_THAN, 5);
        }));
        return;
    }

    //  遍历生成卡片
    for (const auto &p : products) {
        int id = p.product_id;
        std::string name(p.product_name);
        double price = p.price;
        int stock = p.stock;

        // 定义操作按钮
        // 修改按钮
        auto btn_edit = Button(
            "✎ 编辑",
            [this, id, name, price, stock] {
                selected_product_id = id;
                edit_name = name;
                edit_price_str = Utils::format_price(price);
                edit_stock_str = std::to_string(stock);
                status_msg = ""; // 清空错误信息
                show_popup = 1;
            },
            ButtonOption::Ascii());

        // 删除按钮
        auto btn_del = Button(
            "🗑 删除",
            [this, id] {
                selected_product_id = id;
                show_popup = 2;
            },
            ButtonOption::Ascii());

        // 将按钮放入水平容器
        auto btns_layout = Container::Horizontal({btn_edit, btn_del});

        // 渲染单个商品卡片
        auto card_renderer = Renderer(btns_layout, [=] {
            bool is_focused = btns_layout->Focused();
            Color border_color = is_focused
                                     ? static_cast<Color>(Color::Gold1)
                                     : static_cast<Color>(Color::GrayLight);
            Color bg_color = is_focused ? static_cast<Color>(Color::Grey11)
                                        : static_cast<Color>(Color::Default);

            return hbox({// 左侧 ID
                         vbox({text("ID") | dim | center,
                               text(std::to_string(id)) | bold | center |
                                   color(Color::Cyan)}) |
                             size(WIDTH, EQUAL, 6),

                         // 中间 信息
                         vbox({hbox({text("商品: ") | dim, text(name) | bold}),
                               hbox({text("库存: ") | dim,
                                     text(std::to_string(stock)) |
                                         color(stock < 10 ? Color::Red
                                                          : Color::Green)})}) |
                             flex,

                         // 右侧 价格
                         vbox({text("价格") | dim | center,
                               text("￥" + Utils::format_price(price)) |
                                   color(Color::Yellow) | bold}) |
                             size(WIDTH, GREATER_THAN, 10),

                         separator(),

                         // 操作按钮区
                         vbox({btn_edit->Render() | size(WIDTH, EQUAL, 10),
                               btn_del->Render() | size(WIDTH, EQUAL, 10)}) |
                             center}) |
                   borderRounded | color(border_color) | bgcolor(bg_color);
        });

        list_container->Add(card_renderer);
    }
}
