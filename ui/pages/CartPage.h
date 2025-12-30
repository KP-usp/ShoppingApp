#include "AppContext.h"
#include <deque>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory.h>

using namespace ftxui;

class CartLayOut {
  private:
    Component component;

    // 选择送达时间(默认特快送达)
    std::vector<int> delivery_selections;

    // 商品购买数量
    std::vector<int> quantities;

    // 是否选中商品, 1 代表选中而 0 代表未选中
    std::deque<bool> is_chosen;

    // 选择的支付方式（0-支付宝 1-微信 2-银行卡）
    int payment_method = 0;

    // 切换弹窗以及背景页面
    int show_popup =
        0; // 0-无弹窗 1-支付弹窗 2-提示支付成功弹窗 3-提示未选择商品弹窗

    const std::vector<std::string> delivery_choices = {
        "普通递送（五天后送达，免费）",
        "普快递送（三天后送达， 费用 3 元）",
        "特快递送（一日送达, 费用 6 元) ",
    };

    const std::vector<std::string> payment_choices = {
        "支付宝",
        "微信支付",
        "中国银行卡",
    };

    // 包含渲染页面的主逻辑
    void init_page(AppContext &ctx, std::function<void()> on_shopping,
                   std::function<void()> on_orders_info,
                   std::function<void()> delete_item_success,
                   std::function<void()> checkout_success);

  public:
    // 构造器：创建商城页面组件，并通过接受购买函数跳转商城页面
    CartLayOut(AppContext &ctx, std::function<void()> on_shopping,
               std::function<void()> on_orders_info,
               std::function<void()> delete_item_success,
               std::function<void()> checkout_success) {
        init_page(ctx, on_shopping, on_orders_info, delete_item_success,
                  checkout_success);
    }

    Component get_component() { return component; }

    void refresh(AppContext &ctx, std::function<void()> on_shopping,
                 std::function<void()> on_orders_info,
                 std::function<void()> delete_item_success,
                 std::function<void()> checkout_success) {

        // 先清空容器，并重置 vector 类成员
        component->DetachAllChildren();
        delivery_selections.clear();
        quantities.clear();
        is_chosen.clear();

        init_page(ctx, on_shopping, on_orders_info, delete_item_success,
                  checkout_success);
    }
};
