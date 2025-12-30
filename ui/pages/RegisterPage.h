#include "AppContext.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory.h>

using namespace ftxui;

class RegisterLayOut {
  private:
    // 控制弹窗的出现
    int show_popup = 0;

    std::shared_ptr<std::string> username = std::make_shared<std::string>();
    std::shared_ptr<std::string> password = std::make_shared<std::string>();
    std::shared_ptr<std::string> again_password =
        std::make_shared<std::string>();
    std::shared_ptr<std::string> message = std::make_shared<std::string>();

    Component component;

  public:
    RegisterLayOut(AppContext &ctx, std::function<void()> on_register_success,
                   std::function<void()> on_login);

    Component get_component() { return component; }

    void clear() {
        *username = "";
        *password = "";
        *again_password = "";
        *message = "";
        show_popup = 0;
    }
};
