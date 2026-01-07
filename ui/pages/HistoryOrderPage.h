#pragma once

#include "AppContext.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory.h>

using namespace ftxui;

class HistoryOrderLayOut {
  private:
    Component component;

    int show_popup = 0; // 弹窗 index  0 - 历史订单页 1 - 是否清除全部历史订单

    const std::vector<std::string> delivery_choices = {
        "普通递送(免费，五天抵达)",
        "普快递送(3 元, 三天抵达）",
        "特快递送(6 元, 一日抵达)) ",
    };

    const std::vector<int> delivery_required_time = {5, 3, 1};

  public:
    // 构造器：创建历史订单页面组件
    HistoryOrderLayOut(AppContext &ctx, std::function<void()> on_orders_info,
                       std::function<void()> on_shopping,
                       std::function<void()> refresh_history_order_page) {

        init_page(ctx, on_orders_info, on_shopping, refresh_history_order_page);
    }

    Component get_component() { return component; }

    void init_page(AppContext &ctx, std::function<void()> on_orders_info,
                   std::function<void()> on_shopping,
                   std::function<void()> refresh_history_order_page);

    void refresh(AppContext &ctx, std::function<void()> on_orders_info,
                 std::function<void()> on_shopping,
                 std::function<void()> refresh_history_order_page) {
        show_popup = 0;
        component->DetachAllChildren();

        init_page(ctx, on_orders_info, on_shopping, refresh_history_order_page);
    }
};
