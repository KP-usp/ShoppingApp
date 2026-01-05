file(GLOB_RECURSE UI_SOURCES "*.cpp")

add_library(shopping_ui STATIC ${UI_SOURCES})

target_include_directories(shopping_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} pages
                                              components admin/pages admin)

target_link_libraries(shopping_ui PUBLIC ui_utils shopping_model ftxui::screen
                                         ftxui::dom ftxui::component)
