# 接口库（只含有头文件）
add_library(model_utils INTERFACE)

# 宏定义：定义数据库的路径
target_compile_definitions(model_utils
                           INTERFACE DATA_PATH="${PROJECT_SOURCE_DIR}/data")

target_include_directories(model_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
