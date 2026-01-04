#include "RegisterPage.h"
#include "SharedComponent.h"

RegisterLayOut::RegisterLayOut(AppContext &ctx,
                               std::function<void()> on_register_success,
                               std::function<void()> on_login) {

    Component input_username = SharedComponents::create_input_with_placeholder(
        username.get(), "请输入新用户名", false);
    Component input_password = SharedComponents::create_input_with_placeholder(
        password.get(), "请输入新密码", true);
    Component input_again_password =
        SharedComponents::create_input_with_placeholder(
            again_password.get(), "请再次输入新密码", true);

    // 提示注册成功弹窗
    Component hint_popup = Button("确定", [this, on_register_success] {
        show_popup = 0;
        on_register_success(); // 用户点击确定后才跳转
    });

    Component btn_register = Button("注册", [&ctx, this, on_register_success] {
        if (ctx.user_manager.check_register(*username, *password,
                                            *again_password,
                                            *message) == Result::SUCCESS) {
            show_popup = 1;
        }
    });

    // 用来返回登录界面
    Component btn_login = Button("返回登录", [on_login] { on_login(); });

    auto register_container = Container::Vertical({
        input_username,
        input_password,
        input_again_password,
        btn_register,
        btn_login,
    });

    auto final_container = Container::Tab(
        {
            register_container,
            hint_popup,
        },
        (int *)&show_popup);

    this->component = Renderer(final_container, [=] {
        auto background =
            vbox({
                text("用户注册") | bold | center,
                separator(),
                hbox({
                    text("新用户名:   ") | center,
                    input_username->Render(),
                }) | size(WIDTH, GREATER_THAN, 30),
                hbox({
                    text("新密码:     ") | center,
                    input_password->Render(),
                }) | size(WIDTH, GREATER_THAN, 30),

                hbox({
                    text("验证新密码: ") | center,
                    input_again_password->Render(),
                }) | size(WIDTH, GREATER_THAN, 30),

                separator(),
                hbox({
                    filler(),
                    btn_register->Render(),
                    filler(),
                    btn_login->Render(),
                    filler(),
                }),
                text(*message) | color(ftxui::Color::Red) | center,
            }) |
            border | center;

        if (show_popup == 1) {
            auto popup_content = vbox({text("注册成功") | center, separator(),
                                       hint_popup->Render() | center |
                                           size(WIDTH, GREATER_THAN, 10)});

            auto popup_window = window(text("提示"), popup_content);
            return dbox({background, popup_window |
                                         size(WIDTH, GREATER_THAN, 40) |
                                         size(HEIGHT, GREATER_THAN, 8) |
                                         clear_under | center});
        }
        return background;
    });
}
