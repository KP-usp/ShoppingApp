file(GLOB MODEL_SOURCES "*.cpp")

add_library(shopping_model STATIC ${MODEL_SOURCES})

target_include_directories(shopping_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# -- 寻找 MySQL 库

# 寻找头文件路径
find_path(
  MYSQL_INCLUDE_DIR
  NAMES mysql.h
  PATHS /usr/include/mysql /usr/local/include/mysql)

# 寻找库文件路径
find_library(
  MYSQL_LIBRARY
  NAMES mysqlclient
  PATHS /usr/lib/x86_64-linux-gnu /usr/local/lib)

# 检查是否找到并创建目标
if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
  message(STATUS "Found MySQL Include: ${MYSQL_INCLUDE_DIR}")
  message(STATUS "Found MySQL Library: ${MYSQL_LIBRARY}")

  # 创建一个现代 CMake 目标
  add_library(MySQL::MySQL UNKNOWN IMPORTED)
  set_target_properties(
    MySQL::MySQL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
                            IMPORTED_LOCATION "${MYSQL_LIBRARY}")
else()
  message(
    FATAL_ERROR
      "Could not find libmysqlclient-dev. Install it with: sudo apt install libmysqlclient-dev"
  )
endif()

# 连接 MySQL 库 和 其他依赖
target_link_libraries(
  shopping_model
  PUBLIC logger
  PRIVATE MySQL::MySQL)
