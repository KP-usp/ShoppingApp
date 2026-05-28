file(GLOB MODEL_SOURCES "*.cpp")

add_library(shopping_model STATIC ${MODEL_SOURCES})

target_include_directories(shopping_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenSSL REQUIRED)

target_link_libraries(
  shopping_model
  PUBLIC model_utils OpenSSL::SSL OpenSSL::Crypto
  PUBLIC database
  )

