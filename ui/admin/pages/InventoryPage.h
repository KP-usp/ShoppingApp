#pragma once
#include "AppContext.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

using namespace ftxui;

class InventoryLayOut {
  private:
    Component component;

    // 搜索框状态
    std::string search_query;

    // 编辑框状态 (因为 Input 需要 string，所以我们要把 int/double 转 string)
    std::string edit_name;
    std::string edit_price_str;
    std::string edit_stock_str;

    // 当前选中的商品 ID
    int selected_product_id = -1;

    // 弹窗状态: 0-列表页, 1-编辑弹窗, 2-删除确认弹窗
    int show_popup = 0;

    // 错误/提示信息
    std::string status_msg;

  public:
    InventoryLayOut(AppContext &ctx, std::function<void()> back_dashboard,
                    std::function<void()> refresh_invnetory_page) {
        init_page(ctx, back_dashboard, refresh_invnetory_page);
    }

    void init_page(AppContext &ctx, std::function<void()> back_dashboard,
                   std::function<void()> refresh_invnetory_page);

    // 刷新列表逻辑
    void refresh_list(AppContext &ctx, Component list_container,
                      std::function<void()> back_dashboard);

    Component get_component() { return component; }

    void refresh(AppContext &ctx, std::function<void()> back_dashboard,
                 std::function<void()> refresh_invnetory_page) {
        init_page(ctx, back_dashboard, refresh_invnetory_page);
    }
};
