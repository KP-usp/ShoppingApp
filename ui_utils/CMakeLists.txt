file(GLOB UTILS_SOURCES "*.cpp")

add_library(ui_utils STATIC ${UTILS_SOURCES})

target_include_directories(ui_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ui_utils INTERFACE shopping_model)
