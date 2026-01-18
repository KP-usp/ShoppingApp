file(GLOB LOGGER_SOURCES "*.cpp")

add_library(logger STATIC ${LOGGER_SOURCES})

target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(logger PUBLIC model_utils)
