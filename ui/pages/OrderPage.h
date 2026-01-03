#include "AppContext.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory.h>

using namespace ftxui;

class OrderLayOut {
  private:
    Component component;

    // 存储用户输入的新地址
    std::string new_address;

    // 格式检查: 新地址为空或大于 50 字符提示报错
    std::string status_text;

    // 当前正在操作的订单 ID （用于修改或取消）
    long long temp_selected_order_id = -1;

    // 临时存储配送方式的选择索引
    int temp_selected_delivery_idx = 0;

    // 弹窗 index
    int show_popup =
        0; // 0-订单页 1-选择修改弹窗（地址或递送服务） 2-地址修改弹窗
           // 3-地址修改成功弹窗  4-递送服务修改弹窗 5-递送服务修改成功弹窗

    const std::vector<std::string> delivery_choices = {
        "普通递送(免费，五天抵达)",
        "普快递送(3 元, 三天抵达）",
        "特快递送(6 元, 当天抵达)) ",
    };

    const std::vector<int> delivery_required_time = {5, 3, 0};

  public:
    // 构造器：创建商城页面组件，并通过接受购买函数跳转商城页面
    OrderLayOut(AppContext &ctx, std::function<void()> on_checkout,
                std::function<void()> on_shopping,
                std::function<void()> on_history_orders_info,
                std::function<void()> on_orders_update) {

        init_page(ctx, on_checkout, on_shopping, on_history_orders_info,
                  on_orders_update);
    }

    Component get_component() { return component; }

    void init_page(AppContext &ctx, std::function<void()> on_checkout,
                   std::function<void()> on_shopping,
                   std::function<void()> on_history_orders_info,
                   std::function<void()> on_orders_update);

    void refresh(AppContext &ctx, std::function<void()> on_checkout,
                 std::function<void()> on_shopping,
                 std::function<void()> on_history_orders_info,
                 std::function<void()> on_orders_update) {
        component->DetachAllChildren();
        new_address = "";
        status_text = "";
        temp_selected_order_id = -1;
        temp_selected_delivery_idx = 0;

        init_page(ctx, on_checkout, on_shopping, on_history_orders_info,
                  on_orders_update);
    }
};
