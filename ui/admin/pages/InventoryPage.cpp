#include "InventoryPage.h"
#include "SharedComponent.h"
#include <vector>

void InventoryLayOut::init_page(AppContext &ctx,
                                std::function<void()> back_dashboard,
                                std::function<void()> refresh_inventory_page) {
    // 加载商品数据
    ctx.product_manager.load_all_product();

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

    // 添加商品按钮
    auto btn_add_new = Button(
        "➕ 添加商品",
        [this] {
            // 清空输入框
            new_prod_name = "";
            new_prod_price_str = "";
            new_prod_stock_str = "";
            status_msg = "";
            show_popup = 4; // 跳转添加弹窗
        },
        ButtonOption::Animated(Color::Cyan));

    // 返回仪表盘按钮
    auto btn_back = Button("返回控制台", back_dashboard,
                           ButtonOption::Animated(Color::RedLight));

    // 定义弹窗组件 (修改编辑模式)
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

    // 定义弹窗组件（恢复确认）
    auto btn_confirm_restore = Button(
        "确认恢复",
        [this, &ctx, list_container, back_dashboard, refresh_inventory_page] {
            ctx.product_manager.restore_product(selected_product_id);
            show_popup = 0;
            refresh_list(ctx, list_container, back_dashboard);
            refresh_inventory_page();
        },
        ButtonOption::Animated(Color::Green));

    auto btn_cancel_restore = Button("取消", [this] { show_popup = 0; });
    auto restore_container =
        Container::Horizontal({btn_confirm_restore, btn_cancel_restore});

    // 定义弹窗组件（添加编辑模式）
    auto input_add_name = Input(&new_prod_name, "输入商品名称");
    auto input_add_price = Input(&new_prod_price_str, "输入价格");
    auto input_add_stock = Input(&new_prod_stock_str, "输入库存");

    auto btn_save_add = Button(
        "确认添加",
        [this, &ctx, list_container, back_dashboard, refresh_inventory_page] {
            try {
                if (new_prod_name.empty()) {
                    status_msg = "商品名不能为空";
                    return;
                }
                double price = std::stod(new_prod_price_str);
                int stock = std::stoi(new_prod_stock_str);

                auto res = ctx.product_manager.add_product(new_prod_name, price,
                                                           stock);

                if (res == FileErrorCode::OK) {
                    show_popup = 0;
                    refresh_list(ctx, list_container, back_dashboard);
                    refresh_inventory_page();
                } else {
                    status_msg = "添加失败";
                }
            } catch (...) {
                status_msg = "格式错误，价格和库存必须是数字";
            }
        },
        ButtonOption::Animated(Color::Green));

    auto btn_cancel_add = Button("取消", [this] { show_popup = 0; });

    auto add_container = Container::Vertical(
        {input_add_name, input_add_price, input_add_stock,
         Container::Horizontal({btn_save_add, btn_cancel_add})});

    std::string path = "data/debug.log";
    std::ofstream outfile1(path, std::ios_base::app);
    if (outfile1.is_open()) {
        outfile1 << "加载列表之前" << std::endl;
        outfile1.close();
    }

    // --- 初始加载列表 ---
    refresh_list(ctx, list_container, back_dashboard);

    std::ofstream outfile(path, std::ios_base::app);
    if (outfile.is_open()) {
        outfile << "加载列表之后" << std::endl;
        outfile.close();
    }

    // --- 组装主页面结构 ---
    auto top_bar =
        Container::Horizontal({search_input | flex, btn_search, btn_add_new});
    // 支持鼠标滚动进度条
    auto scroller = SharedComponents::Scroller(list_container);
    auto final_logic_content =
        Container::Vertical({top_bar, scroller | flex, btn_back});

    auto final_main_layout =
        SharedComponents::allow_scroll_action(final_logic_content);

    // --- Tab 容器管理页面和弹窗 ---
    auto tab_container = Container::Tab(
        {
            final_main_layout, // 0
            edit_container,    // 1
            del_container,     // 2
            restore_container, // 3
            add_container      // 4
        },
        &show_popup);

    // --- 最终渲染器 ---
    this->component = Renderer(tab_container, [=] {
        // 背景层 (列表页)
        auto background = vbox(
            {vbox({
                 text(" ") | size(HEIGHT, EQUAL, 1),
                 text(" 商   品   库   存   管   理 ") | bold | center |
                     color(Color::Red),
                 text(" —— INVENTORY MANAGEMENT SYSTEM —— ") | dim | center |
                     color(Color::GrayLight),
                 text(" ") | size(HEIGHT, EQUAL, 1),
             }) | borderDouble |
                 color(Color::Red),

             // 搜索栏区域
             hbox({text("🔍 ") | center,
                   search_input->Render() | borderRounded | flex,
                   btn_search->Render(), text("  "), btn_add_new->Render()}) |
                 size(HEIGHT, EQUAL, 3),

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

        // 统一的弹窗样式生成器
        auto make_popup = [&](std::string title, Element content,
                              Color border_c) {
            return dbox(
                {background,
                 window(text(" " + title + " "),
                        vbox({content | center, separator(),
                              text(status_msg) | color(Color::Red) | center})) |
                     size(WIDTH, GREATER_THAN, 50) | center | clear_under});
        };

        if (show_popup == 1) { // 修改编辑
            return make_popup(
                "修改商品信息",
                vbox({hbox({text("名称: "), input_edit_name->Render() | flex}),
                      text(" "),
                      hbox({text("价格: "), input_edit_price->Render() | flex}),
                      text(" "),
                      hbox({text("库存: "), input_edit_stock->Render() | flex}),
                      separator(),
                      hbox({btn_save_edit->Render() | flex,
                            btn_cancel_edit->Render() | flex})}),
                Color::Cyan);
        } else if (show_popup == 2) { // 删除
            return make_popup(
                "危险操作警告",
                vbox({text("确定要删除 ID: " +
                           std::to_string(selected_product_id) + " 吗？") |
                          center,
                      text("商品下架后用户将无法购买") | color(Color::Red) |
                          center,
                      separator(), del_container->Render() | center}),
                Color::Red);
        } else if (show_popup == 3) { //  恢复
            return make_popup(
                "恢复上架",
                vbox({text("确定要恢复 ID: " +
                           std::to_string(selected_product_id) + " 吗？") |
                          center,
                      text("商品将重新对用户可见") | color(Color::Green) |
                          center,
                      separator(), restore_container->Render() | center}),
                Color::Green);
        } else if (show_popup == 4) { //  添加
            return make_popup(
                "上架新商品",
                vbox({hbox({text("名称: "), input_add_name->Render() | flex}),
                      text(" "),
                      hbox({text("价格: "), input_add_price->Render() | flex}),
                      text(" "),
                      hbox({text("库存: "), input_add_stock->Render() | flex}),
                      separator(),
                      hbox({btn_save_add->Render() | flex,
                            btn_cancel_add->Render() | flex})}),
                Color::Cyan);
        }
        return background;
    });
}

void InventoryLayOut::refresh_list(AppContext &ctx, Component list_container,
                                   std::function<void()> on_back) {
    list_container->DetachAllChildren();

    //  获取搜索结果
    auto products = ctx.product_manager.search_all_product(search_query);

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
        ProductStatus status = p.status;
        bool is_deleted = (status == ProductStatus::DELETED);

        Component btns_layout;

        if (!is_deleted) {
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
            btns_layout = Container::Horizontal({btn_edit, btn_del});
        } else {
            // [已删除商品]: 显示 恢复
            auto btn_restore = Button(
                "♻ 恢复",
                [this, id] {
                    selected_product_id = id;
                    show_popup = 3;
                },
                ButtonOption::Ascii());

            btns_layout = Container::Horizontal({btn_restore});
        }

        // 渲染单个商品卡片
        auto card_renderer = Renderer(btns_layout, [=] {
            bool is_focused = btns_layout->Focused();

            // 样式区分：已删除的商品变暗

            Color border_color =
                is_focused ? static_cast<Color>(Color::Gold1)
                           : (is_deleted ? Color::GrayDark : Color::GrayLight);

            Color text_color = is_deleted ? Color::GrayDark : Color::White;

            Color bg_color = is_focused ? static_cast<Color>(Color::Grey11)
                                        : static_cast<Color>(Color::Default);

            // 状态标签
            auto status_tag = is_deleted
                                  ? text(" [已下架] ") | color(Color::Red)
                                  : text(" [在售] ") | color(Color::Green);

            return hbox({// 左侧 ID
                         vbox({text("ID") | dim | center,
                               text(std::to_string(id)) | bold | center |
                                   color(Color::Cyan)}) |
                             size(WIDTH, EQUAL, 6),

                         // 中间 信息
                         vbox({hbox({text(name) | bold | color(text_color),
                                     status_tag}),
                               hbox({text("库存: ") | dim,
                                     text(std::to_string(stock)) |
                                         color(is_deleted
                                                   ? Color::GrayDark
                                                   : (stock < 10
                                                          ? Color::Red
                                                          : Color::Green))})}) |
                             flex,

                         // 右侧 价格
                         vbox({text("价格") | dim | center,
                               text("￥" + Utils::format_price(price)) |
                                   color(is_deleted ? Color::GrayDark
                                                    : Color::Yellow) |
                                   bold}) |
                             size(WIDTH, GREATER_THAN, 10),

                         separator(),

                         // 操作按钮区 (自动根据 btns_layout 渲染)
                         btns_layout->Render() | center |
                             size(WIDTH, GREATER_THAN, 20)

                   }) |
                   borderRounded | color(border_color) | bgcolor(bg_color);
        });

        list_container->Add(card_renderer);
    }
}
