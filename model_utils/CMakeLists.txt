file(GLOB MODEL_UTILS_SOURCES "*.cpp")

add_library(model_utils STATIC ${MODEL_UTILS_SOURCES})

# 宏定义：定义数据库的路径
target_compile_definitions(model_utils
                           PUBLIC DATA_PATH="${PROJECT_SOURCE_DIR}/data")

target_include_directories(model_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
