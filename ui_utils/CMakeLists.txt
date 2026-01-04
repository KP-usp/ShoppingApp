add_library(ui_utils INTERFACE)

target_include_directories(ui_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  ui_utils INTERFACE httplib::httplib nlohmann_json::nlohmann_json OpenSSL::SSL
                     OpenSSL::Crypto Threads::Threads shopping_model)
