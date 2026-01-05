#pragma once
#include "AppContext.h"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

class InventoryLayOut {
  private:
    Component component;

  public:
    InventoryLayOut(AppContext &ctx, std::function<void()> back_dashboard) {
        init_page(ctx, back_dashboard);
    }

    void init_page(AppContext &ctx, std::function<void()> back_dashboard) {
        auto container = Container::Vertical({});
        this->component = Renderer(container, [] { return text("敬请期待"); });
    };

    Component get_component() { return component; }

    void refresh(AppContext &ctx, std::function<void()> back_dashboard) {
        init_page(ctx, back_dashboard);
    }
};
