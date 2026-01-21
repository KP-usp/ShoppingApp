file(GLOB MODEL_SOURCES "*.cpp")

add_library(shopping_model STATIC ${MODEL_SOURCES})

target_include_directories(shopping_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# -- 寻找 MySQL 库

# 1. 寻找 C++ 驱动的头文件 (通常在 include 或 include/cppconn 下) 关键文件是 mysql_driver.h 或
#   cppconn/driver.h
find_path(
  MYSQL_CPP_INCLUDE_DIR
  NAMES mysql_driver.h mysql_connection.h cppconn/driver.h
  PATHS /usr/include /usr/local/include /usr/include/mysql-cppconn-8
  PATH_SUFFIXES cppconn)

# 1. 寻找 C++ 驱动的库文件 (名字是 mysqlcppconn，不是 mysqlclient)
find_library(
  MYSQL_CPP_LIBRARY
  NAMES mysqlcppconn
  PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib)

# 1. 检查并创建 Target
if(MYSQL_CPP_INCLUDE_DIR AND MYSQL_CPP_LIBRARY)
  message(STATUS "Found MySQL Connector/C++ Include: ${MYSQL_CPP_INCLUDE_DIR}")
  message(STATUS "Found MySQL Connector/C++ Library: ${MYSQL_CPP_LIBRARY}")

  add_library(MySQL::CppConn UNKNOWN IMPORTED)
  set_target_properties(
    MySQL::CppConn
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_CPP_INCLUDE_DIR}"
               IMPORTED_LOCATION "${MYSQL_CPP_LIBRARY}")
else()
  message(
    FATAL_ERROR
      "Could not find libmysqlcppconn. Please install it with:\n sudo apt install libmysqlcppconn-dev"
  )
endif()

# 连接 MySQL 库 和 其他依赖
target_link_libraries(
  shopping_model
  PUBLIC model_utils
  PUBLIC database
  PRIVATE MySQL::CppConn)
