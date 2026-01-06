#include "LoginPage.h"
#include "SharedComponent.h"

LoginLayOut::LoginLayOut(AppContext &ctx, std::function<void()> onLoginSuccess,
                         std::function<void()> on_register) {

    // 用户名框
    Component input_username = SharedComponents::create_input_with_placeholder(
        username.get(), "请输入用户名", false);

    Component input_password = SharedComponents::create_input_with_placeholder(
        password.get(), "请输入密码", true);

    // 登录按钮
    Component btn_login = Button("登录", [&ctx, this, onLoginSuccess] {
        if (ctx.user_manager.check_login(*username, *password) ==
            Result::SUCCESS) {
            *message = "";
            ctx.current_user = ctx.user_manager.get_current_user();

            onLoginSuccess();
        } else {
            *message =
                "登录失败，用户名或密码错误, 如果忘记密码请联系管理员重置";
        }
    });

    // 注册按钮
    auto btn_register = Button("注册", [on_register] { on_register(); });

    // 布局
    auto layout = Container::Vertical({
        input_username,
        input_password,
        btn_login,
        btn_register,
    });

    // 渲染器（renderer）
    // 把组件变成漂亮的 ui 界面
    this->component = Renderer(layout, [=] {
        return vbox({
                   text("用户登录") | bold | center,
                   separator(),
                   hbox({text("账号: ") | center, input_username->Render()}) |
                       size(WIDTH, GREATER_THAN, 30),
                   hbox({text("密码: ") | center, input_password->Render()}) |
                       size(WIDTH, GREATER_THAN, 30),
                   separator(),
                   hbox({
                       filler(),
                       btn_login->Render(),
                       filler(),
                       btn_register->Render(),
                       filler(),
                   }),

                   text(*message) | color(ftxui::Color::Red) | center,
               }) |
               border | center;
    });
}
