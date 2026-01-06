#include "UserManagePage.h"
#include "InventoryPage.h"
#include "SharedComponent.h"
#include <vector>

void UserManageLayOut::init_page(
    AppContext &ctx, std::function<void()> back_dashboard,
    std::function<void()> refresh_user_manage_page) {
    // UI 容器
    auto list_container = Container::Vertical({});

    // 搜索组件
    auto search_input = Input(&search_query, "输入 ID 或 用户名进行搜索...");

    // 搜索按钮
    auto btn_search = Button(
        "🔍 搜索",
        [this, &ctx, list_container, back_dashboard] {
            refresh_list(ctx, list_container, back_dashboard);
        },
        ButtonOption::Animated(Color::Gold1));

    // 返回按钮
    auto btn_back = Button("返回控制台", back_dashboard,
                           ButtonOption::Animated(Color::RedLight));

    // 编辑弹窗组件
    auto input_edit_password =
        Input(&edit_password, "输入新密码 (留空则不修改)");

    // 权限切换
    auto checkbox_is_admin = Checkbox("授予管理员权限", &edit_is_admin);

    // 修改按钮
    auto btn_save_edit = Button(
        "保存修改",
        [this, &ctx, list_container, back_dashboard, refresh_user_manage_page] {
            auto user_opt = ctx.user_manager.get_user_by_id(selected_user_id);
            if (!user_opt.has_value()) {
                status_msg = "错误：找不到该用户";
                return;
            }
            User current_user = user_opt.value();

            // 修改属性
            current_user.is_admin = edit_is_admin;

            // 如果输入了新密码，则进行加密并更新
            if (!edit_password.empty()) {
                std::string error;
                if (UserManager::is_valid_password_format(
                        edit_password, error) == Result::FAILURE) {
                    status_msg = error;
                    return;
                }
                current_user.password =
                    SecurityUtils::hash_password(edit_password);
            }

            //  保存
            FileErrorCode code = ctx.user_manager.update_user(current_user);

            if (code == FileErrorCode::OK) {
                status_msg = "";
                edit_password = ""; // 清空敏感信息
                refresh_list(ctx, list_container, back_dashboard);
                refresh_user_manage_page();
                show_popup = 0;
            } else {
                status_msg = "数据库写入失败";
            }
        },
        ButtonOption::Animated(Color::Green));

    auto btn_cancel_edit = Button("取消", [this] { show_popup = 0; });

    auto edit_container = Container::Vertical(
        {input_edit_password, checkbox_is_admin,
         Container::Horizontal({btn_save_edit, btn_cancel_edit})});

    // 删除确认弹窗
    auto btn_confirm_del = Button(
        "确认封禁/删除",
        [this, &ctx, list_container, back_dashboard, refresh_user_manage_page] {
            ctx.user_manager.delete_user(selected_user_id);
            refresh_list(ctx, list_container, back_dashboard);
            refresh_user_manage_page();
            show_popup = 0;
        },
        ButtonOption::Animated(Color::Red));

    auto btn_cancel_del = Button("取消", [this] { show_popup = 0; });

    auto del_container =
        Container::Horizontal({btn_confirm_del, btn_cancel_del});

    // 初始加载
    refresh_list(ctx, list_container, back_dashboard);

    // 布局组装
    // 支持滚动
    auto scroller = SharedComponents::Scroller(list_container);
    auto main_logic_content = Container::Vertical({scroller, btn_back});
    auto main_view = SharedComponents::allow_scroll_action(main_logic_content);

    auto final_main_layout = Container::Vertical(
        {Container::Horizontal({search_input | flex, btn_search}), main_view});

    auto tab_container = Container::Tab(
        {
            final_main_layout, // 0
            edit_container,    // 1
            del_container,     // 2
        },
        &show_popup);

    // 渲染器
    this->component = Renderer(tab_container, [=] {
        // 背景层
        auto background =
            vbox({vbox({
                      text(" ") | size(HEIGHT, EQUAL, 1),
                      text(" 用   户   权   限   管   理 ") | bold | center |
                          color(Color::Red),
                      text(" —— USER ACCOUNT CONTROL —— ") | dim | center |
                          color(Color::GrayLight),
                      text(" ") | size(HEIGHT, EQUAL, 1),
                  }) | borderDouble |
                      color(Color::Red),

                  // 搜索栏
                  hbox({text("🔍 ") | center,
                        search_input->Render() | borderRounded | flex,
                        btn_search->Render()}),

                  separator(),

                  // 列表区
                  scroller->Render() | flex,

                  separator(),

                  // 底部
                  hbox({
                      filler(),
                      btn_back->Render() | center | size(HEIGHT, EQUAL, 3),
                      filler(),
                  })});

        // 弹窗渲染
        if (show_popup == 1) {
            return dbox(
                {background,
                 window(
                     text(" 编辑用户: " + selected_username_display),
                     vbox({text("修改权限") | bold, checkbox_is_admin->Render(),
                           separator(), text("重置密码") | bold,
                           input_edit_password->Render(),
                           text("注意：若不修改密码请留空") | dim |
                               size(HEIGHT, EQUAL, 1),
                           separator(),
                           text(status_msg) | color(Color::Red) | center,
                           separator(),
                           hbox({btn_save_edit->Render() | flex,
                                 btn_cancel_edit->Render() | flex})})) |
                     size(WIDTH, GREATER_THAN, 50) | center | clear_under});
        } else if (show_popup == 2) {
            return dbox(
                {background,
                 window(text(" 封禁账户警告 "),
                        vbox({text("确定要封禁 ID: " +
                                   std::to_string(selected_user_id) + " 吗？") |
                                  center,
                              text("该用户将无法再登录系统") |
                                  color(Color::Red) | center,
                              separator(), del_container->Render() | center})) |
                     size(WIDTH, GREATER_THAN, 40) | center | clear_under});
        }
        return background;
    });
}

void UserManageLayOut::refresh_list(AppContext &ctx, Component list_container,
                                    std::function<void()> back_dashboard) {
    list_container->DetachAllChildren();

    //  搜索到的数据
    auto users = ctx.user_manager.search_users_list(search_query);

    if (users.empty()) {
        list_container->Add(Renderer([] {
            return text("没有找到符合条件的用户") | center | dim |
                   size(HEIGHT, GREATER_THAN, 5);
        }));
        return;
    }

    //  生成卡片
    for (const auto &u : users) {
        int id = u.id;
        std::string name(u.username);
        bool is_admin = u.is_admin;

        // 标记：防止操作自己
        bool is_self = (ctx.current_user && ctx.current_user->id == id);

        auto btn_edit = Button(
            "✎  设置",
            [this, id, name, is_admin] {
                selected_user_id = id;
                selected_username_display = name;
                edit_password = "";       // 重置密码框
                edit_is_admin = is_admin; // 同步当前状态
                status_msg = "";
                show_popup = 1;
            },
            ButtonOption::Ascii());

        auto btn_del = Button(
            "🚫 封禁",
            [this, id, is_self] {
                if (is_self) {
                    return;
                }
                selected_user_id = id;
                show_popup = 2;
            },
            is_self ? ButtonOption::Simple()
                    : ButtonOption::Ascii()); // 自己不能点

        auto btns_layout = Container::Horizontal({btn_edit, btn_del});

        auto card_renderer = Renderer(btns_layout, [=] {
            bool is_focused = btns_layout->Focused();
            // 修复：强制类型转换
            Color border_color = is_focused
                                     ? static_cast<Color>(Color::Gold1)
                                     : static_cast<Color>(Color::GrayLight);
            Color bg_color = is_focused ? static_cast<Color>(Color::Grey11)
                                        : static_cast<Color>(Color::Default);

            // 角色标识颜色
            Color role_color = is_admin ? Color::RedLight : Color::GreenLight;
            std::string role_text = is_admin ? "管理员" : "普通用户";
            if (is_self)
                role_text += " (您)";

            return hbox({// ID
                         vbox({text("ID") | dim | center,
                               text(std::to_string(id)) | bold | center |
                                   color(Color::Cyan)}) |
                             size(WIDTH, EQUAL, 6),

                         separator(),

                         // 信息
                         vbox({hbox({text("账号: ") | dim, text(name) | bold}),
                               hbox({text("身份: ") | dim,
                                     text(role_text) | color(role_color)})}) |
                             flex,

                         separator(),

                         // 按钮
                         vbox({btn_edit->Render() | size(WIDTH, EQUAL, 10),
                               // 如果是自己，不渲染封禁按钮或者渲染为空白
                               is_self ? text("")
                                       : btn_del->Render() |
                                             size(WIDTH, EQUAL, 10)}) |
                             center

                   }) |
                   borderRounded | color(border_color) | bgcolor(bg_color);
        });

        list_container->Add(card_renderer);
    }
}
