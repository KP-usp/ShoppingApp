#pragma once

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

class DashboradLayOut {
  private:
  public:
    static Component Create(std::function<void()> on_inventory_page,
                            std::function<void()> on_usermanage_page) {
        // 定义跳转商品管理和用户管理页面的按钮
        auto option = ButtonOption::Animated(Color::Red);

        auto btn_inventory = Button("进入商品管理", on_inventory_page, option);
        auto btn_users = Button("进入用户管理", on_usermanage_page, option);

        // 按钮布局
        auto container = Container::Horizontal({btn_inventory, btn_users});

        // 渲染逻辑
        return Renderer(container, [=] {
            // 辅助函数：绘制单个功能卡片
            // title: 标题, icon: 图标, desc: 描述, btn: 按钮组件, is_focused:
            // 是否获焦
            auto make_card = [](std::string title, std::string icon,
                                std::string desc, Component btn,
                                bool is_focused) {
                // 焦点状态决定边框颜色
                Color border_c = is_focused
                                     ? static_cast<Color>(Color::Gold1)
                                     : static_cast<Color>(Color::GrayDark);
                Color title_c = is_focused ? static_cast<Color>(Color::RedLight)
                                           : static_cast<Color>(Color::White);

                return vbox({text(icon) | size(HEIGHT, EQUAL, 2) |
                                 center, // 大图标占位
                             text(title) | bold | center | color(title_c),
                             separator() | color(Color::GrayDark),
                             text(" ") | size(HEIGHT, EQUAL, 1), // Padding
                             text(desc) | dim | center,
                             text(" ") | size(HEIGHT, EQUAL, 1), // Padding
                             filler(),
                             btn->Render() | center, // 按钮放在底部
                             filler()}) |
                       borderRounded | color(border_c) |
                       flex; // 让两个卡片平分宽度
            };

            // 检查当前焦点在哪里，以便高亮对应的卡片
            bool inv_focused = btn_inventory->Focused();
            bool user_focused = btn_users->Focused();

            // 构建主视图
            return vbox({// --- 顶部标题栏 (风格参考 ShopPage) ---
                         vbox({
                             text(" ") | size(HEIGHT, EQUAL, 1),
                             text(" 管   理   员   控   制   台 ") | bold |
                                 center | color(Color::Red),
                             text(" —— SYSTEM ADMINISTRATOR DASHBOARD —— ") |
                                 dim | center | color(Color::GrayLight),
                             text(" ") | size(HEIGHT, EQUAL, 1),
                         }) | borderDouble |
                             color(Color::Red),

                         // --- 中间内容区 ---
                         hbox({// 左侧：商品管理卡片
                               make_card("商品库存管理", "📦",
                                         "上架新品 / 下架旧品 / 调整价格",
                                         btn_inventory, inv_focused),

                               // 中间加一点间距
                               text("  "),

                               // 右侧：用户管理卡片
                               make_card("用户与权限", "👥",
                                         "查看列表 / 封禁账户 / 审计",
                                         btn_users, user_focused)}) |
                             flex, // 增加内边距让卡片不贴边

                         // --- 底部状态栏 ---
                         text(" Tip: 使用左右方向键切换模块，回车键进入, "
                              "或者使用鼠标点击") |
                             center | dim});
        });
    }
};
