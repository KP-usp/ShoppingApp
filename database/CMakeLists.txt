file(GLOB DATABASE_SOURCES "*.cpp")

add_library(database STATIC ${DATABASE_SOURCES})

target_include_directories(database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(database PUBLIC logger)
