#pragma once
#include "Utils.h"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

namespace SharedComponents {

// inline 防止重定义
// 渲染输入框特性的共用函数(包括掩码，占位符淡化，获得焦点加粗)
inline Component create_input_with_placeholder(std::string *content,
                                               std::string placeholder_text,
                                               bool is_password = false) {

    InputOption option;
    option.placeholder = placeholder_text;

    // true 代表掩码
    option.password = is_password;

    // 核心函数，占位符淡化和获得焦点加粗
    option.transform = [](InputState state) {
        Element element = state.element;

        if (state.is_placeholder) {
            element = element | color(Color::GrayDark) | dim;

        } else {
            element = element | color(Color::White);
        }

        if (state.focused) {
            element = element | bold | ftxui::select | bgcolor(Color::Blue);
        }

        return element;
    };

    return Input(content, option);
}

inline Element get_clock_element() {

    std::string time_str = Utils::get_current_time();

    return hbox({text("🕒 ") | bold, text(time_str)}) |
           color(Color::Cyan); // 给他个显眼的青色
}

}; // namespace SharedComponents
