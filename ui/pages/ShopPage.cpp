#include "ShopPage.h"
#include <algorithm>
#include <fstream>
#include <vector>

auto name_style = [](Element e) { return e | flex; };
auto price_style = [](Element e) { return e | size(WIDTH, EQUAL, 15); };
auto qty_style = [](Element e) { return e | size(WIDTH, EQUAL, 20); };

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

        // 商品信息表头（商品名、商品价格、 送达时间选项、购买商品数量）
        auto product_header =
            hbox({text("  商品名称") | vcenter | name_style, separator(),
                  text("单价") | center | price_style, separator(),
                  text("购买数量") | center | qty_style,
                  text(" ") | size(WIDTH, EQUAL, 2)}) |
            size(HEIGHT, EQUAL, 3) | inverted | bold;

        // 创建主容器（垂直布局）, 用来放所有的商品行
        auto main_container = Container::Vertical({});

        // 跳转购物车页面按钮
        auto btn_to_cart =
            Button("前往购物车", [on_checkout] { on_checkout(); });

        // 添加到购物车 按钮
        auto btn_add =
            Button("加入购物车", [&ctx, this, &product_list, add_cart] {
                if (all_of(quantities.begin(), quantities.end(),
                           [](int x) { return x == 0; })) {
                    show_popup = 1;
                } else {

                    for (int i = 0; i < product_list.size(); i++) {
                        int qty = (quantities)[i];

                        if (qty > 0) {
                            Product &p = (product_list)[i];

                            // 添加到购物车数据库中
                            ctx.cart_manager.add_item((*ctx.current_user).id,
                                                      p.product_id, qty);
                            (quantities)[i] = 0;
                        }
                    }
                    // 刷新购物车页面
                    add_cart();
                }
            });

        // 弹窗组件（当用户没有选择任何商品还点击加入购物车进行提示）
        auto hint_popup = Button("确定", [this] { show_popup = 0; });

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

                auto qty_color = has_qty ? static_cast<Color>(Color::GreenLight)
                                         : static_cast<Color>(Color::Default);

                return hbox({
                    // 列1: 商品名
                    text("  " + p.product_name) | vcenter | name_style |
                        color(Color::Blue),

                    separator(),

                    // 列2: 价格
                    text(Utils::format_price(p.price) + " 元") | vcenter |
                        center | color(Color::Yellow) | price_style,

                    separator(),

                    // 列3: 数量控制 (+ 0 -)
                    hbox({btn_increment->Render() | vcenter,
                          text("" + std::to_string(qty)) | center |
                              size(WIDTH, EQUAL, 4) | vcenter | bold |
                              color(qty_color),
                          btn_decrement->Render()}) |
                        center | qty_style | size(HEIGHT, EQUAL, 3),

                    separator(),

                }); // 每一行的边框
            });
            main_container->Add(row_renderer);
        }

        auto action_container =
            Container::Horizontal({btn_add | flex, btn_to_cart | flex});

        auto page_container =
            Container::Vertical({main_container, action_container});

        auto final_container =
            Container::Tab({page_container, hint_popup}, (int *)&show_popup);

        // 最终返回一个可滚动的页面
        this->component = Renderer(final_container, [=] {
            auto background =
                vbox({// 标题栏
                      hbox({text("商 品 列 表") | bold | center}) |
                          borderRounded,

                      // 表头
                      product_header, separator(),

                      // 列表内容区 (带滚动条)
                      main_container->Render() | vscroll_indicator | frame |
                          flex,

                      separator(),

                      // 底部操作区
                      hbox({
                          filler(),
                          btn_add->Render(),
                          filler(),
                          btn_to_cart->Render(),
                          filler(),
                      })

                }) |
                border;
            if (show_popup == 1) {
                auto popup_content =
                    vbox({text("您还没有选择商品") | center, separator(),
                          hint_popup->Render() | center |
                              size(WIDTH, GREATER_THAN, 10)});

                auto popup_window = window(text("提示"), popup_content);
                return dbox({background, popup_window |
                                             size(WIDTH, GREATER_THAN, 40) |
                                             size(HEIGHT, GREATER_THAN, 8) |
                                             clear_under | center});
            } else
                return background;
        });
    } else {
        this->component = Renderer([] {
            return vbox({
                text("商品列表") | center,
                separator(),
                text("暂无商品") | center,
            });
        });
    }
}
