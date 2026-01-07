#include "AppContext.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory.h>

using namespace ftxui;

class ShopLayOut {
  private:
    // 存储用户购买商品数量
    std::vector<int> quantities;

    // 存储数量输入框的文本内容 (UI显示/输入用)
    std::vector<std::string> quantities_str;

    // 当前显示的商品列表
    std::vector<Product> current_products;

    // 搜索框的输入内容
    std::string search_query;

    Component component;

    // 弹窗 index
    int show_popup =
        0; // 0-商�页 1-没选商品添加提示 2-库存不足添加提示 3-数量格式错误

  public:
    // 构造体：创建商城页面组件，并通过接受购买结算函数跳转购物车页面
    ShopLayOut(AppContext &ctx, std::function<void()> on_checkout,
               std::function<void()> add_cart) {
        init_page(ctx, on_checkout, add_cart);
    }

    Component get_component() { return component; }

    // 渲染页面的主逻辑
    void init_page(AppContext &ctx, std::function<void()> on_checkout,
                   std::function<void()> add_cart);

    // 辅助函数：根据当前类成员 current_products 重建 UI 列表
    void rebuild_product_list_ui(Component list_container);

    // 刷新页面
    void refresh(AppContext &ctx, std::function<void()> on_checkout,
                 std::function<void()> add_cart) {
        // 清空容器，重置 vector 成员
        component->DetachAllChildren();
        quantities.clear();
        quantities_str.clear();
        show_popup = 0;
        init_page(ctx, on_checkout, add_cart);
    }
};
