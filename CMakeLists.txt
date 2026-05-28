cmake_minimum_required(VERSION 3.11)
project(ShoppingApp VERSION 1.0)

# 强制使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---------- 使用 find_package 替代 FetchContent ----------
# nlohmann-json (header-only, 但 vcpkg 提供 target)
find_package(nlohmann_json CONFIG REQUIRED)
# cpp-httplib
find_package(httplib CONFIG REQUIRED)   # 注意：vcpkg 的包名叫 cpp-httplib，但 CMake target 是 httplib::httplib
# FTXUI (由三个组件组成)
find_package(ftxui CONFIG REQUIRED COMPONENTS screen dom component)
# ---------------------------------------------------------

# 寻找系统中下载的库（加密 + 线程）
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(model_utils)
add_subdirectory(logger)
add_subdirectory(database)
add_subdirectory(model)
add_subdirectory(ui_utils)
add_subdirectory(ui)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address -g)
  add_link_options(-fsanitize=address)
endif()

add_executable(shopping_app main.cpp)

target_link_libraries(shopping_app PRIVATE shopping_ui)
