#pragma once
#include "AppContext.h"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

class UserManageLayOut {
  private:
    Component component;
    // 搜索框状态
    std::string search_query;

    // 存储修改密码
    std::string edit_password;

    // 存储修改权限
    bool edit_is_admin = false; // 用于 Checkbox

    // 当前选中的用户数据(用户 ID 和 用户名)
    int selected_user_id = -1;
    std::string selected_username_display;

    // 弹窗状态: 0-列表页, 1-编辑弹窗, 2-删除确认弹窗

    int show_popup = 0;

    // 错误/提示信息
    std::string status_msg;

  public:
    UserManageLayOut(AppContext &ctx, std::function<void()> back_dashboard,
                     std::function<void()> refresh_user_manage_page) {
        init_page(ctx, back_dashboard, refresh_user_manage_page);
    }

    void init_page(AppContext &ctx, std::function<void()> back_dashboard,
                   std::function<void()> refresh_user_manage_page);

    // 刷新列表逻辑
    void refresh_list(AppContext &ctx, Component list_container,
                      std::function<void()> back_dashboard);

    Component get_component() { return component; }

    void refresh(AppContext &ctx, std::function<void()> back_dashboard,
                 std::function<void()> refresh_user_manage_page) {
        init_page(ctx, back_dashboard, refresh_user_manage_page);
    }
};
