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
            element = element | color(Color::Cyan);
        }

        if (state.focused) {
            element = element | bold | ftxui::select | bgcolor(Color::White);
        }

        return element;
    };

    return Input(content, option);
}

inline Element get_clock_element() {

    std::string time_str = Utils::get_current_time();

    return hbox({text("现在时间: ") | bold, text(time_str)}) |
           color(Color::Cyan);
}

// 用来包裹容器，使其支持鼠标滚轮和滚动条
inline Component Scroller(Component child) {
    auto renderer = Renderer(child, [child] {
        return child->Render() | vscroll_indicator | frame | flex;
    });

    return CatchEvent(renderer, [child](Event event) {
        if (event.is_mouse()) {
            if (event.mouse().button == Mouse::WheelDown) {
                child->OnEvent(Event::ArrowDown);
                return true;
            }
            if (event.mouse().button == Mouse::WheelUp) {
                child->OnEvent(Event::ArrowUp);
                return true;
            }

            if (event.mouse().button == Mouse::Left) {
                if (child->OnEvent(event)) {
                    return true;
                }

                return true;
            }
        }
        return child->OnEvent(event);
    });
}
}; // namespace SharedComponents
