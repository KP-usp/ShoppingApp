#pragma once

#include "AppContext.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory.h>

using namespace ftxui;

class LoginLayOut {
  private:
    // 使用共享指针管理字符串变量的生命周期（之后可重构为类成员）
    std::shared_ptr<std::string> username = std::make_shared<std::string>();
    std::shared_ptr<std::string> password = std::make_shared<std::string>();
    std::shared_ptr<std::string> message = std::make_shared<std::string>();

    // 保存好构建的组件
    Component component;

  public:
    // 构造器：创建登入页面组件，并通过接受登入成功函数告知主 Ui 跳转
    LoginLayOut(AppContext &ctx, std::function<void()> onLoginSuccess,
                std::function<void()> on_register);
    Component get_component() { return component; }

    void clear() {
        *username = "";
        *password = "";
        *message = "";
    }
};
