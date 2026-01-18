cmake_minimum_required(VERSION 3.11)
project(ShoppingApp VERSION 1.0)

# 强制使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -- 外部库-- 引入 (1) nlohmann/json
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2)

# (2) cpp-httplib (网络请求)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
  GIT_TAG v0.14.1)

# (3) FTXUI (界面)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v5.0.0)

FetchContent_MakeAvailable(json httplib ftxui)

# 寻找系统中下载的库（加密 + 线程）
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(model_utils)
add_subdirectory(logger)
add_subdirectory(model)
add_subdirectory(ui_utils)
add_subdirectory(ui)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address -g)
  add_link_options(-fsanitize=address)
endif()

add_executable(shopping_app main.cpp)

target_link_libraries(shopping_app PRIVATE shopping_ui)
