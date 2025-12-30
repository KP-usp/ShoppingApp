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

    const std::vector<std::string> delivery_choices = {
        "普通递送(免费，五天抵达)",
        "普快递送(3 元, 三天抵达）",
        "特快递送(6 元, 当天抵达)) ",
    };

    const std::vector<int> delivery_required_time = {5, 3, 0};

  public:
    // 构造器：创建商城页面组件，并通过接受购买函数跳转商城页面
    OrderLayOut(AppContext &ctx, std::function<void()> on_checkout,
                std::function<void()> on_shopping) {

        init_page(ctx, on_checkout, on_shopping);
    }

    Component get_component() { return component; }

    void init_page(AppContext &ctx, std::function<void()> on_checkout,
                   std::function<void()> on_shopping);

    void refresh(AppContext &ctx, std::function<void()> on_checkout,
                 std::function<void()> on_shopping) {
        component->DetachAllChildren();

        init_page(ctx, on_checkout, on_shopping);
    }
};
