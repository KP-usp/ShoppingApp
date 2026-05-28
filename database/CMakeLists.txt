file(GLOB DATABASE_SOURCES "*.cpp")

add_library(database STATIC ${DATABASE_SOURCES})

target_include_directories(database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

target_link_libraries(
  database
  PUBLIC logger
  PUBLIC unofficial::mysql-connector-cpp::connector
  PUBLIC protobuf::libprotobuf-lite
  PUBLIC rapidjson
  PUBLIC ZLIB::ZLIB
  PUBLIC lz4::lz4
  PUBLIC zstd::libzstd
  )

# 修正链接顺序：将 OpenSSL 追加到 connector 的 INTERFACE_LINK_LIBRARIES，
# 使其在链接命令中出现在 connector 之后
set_property(TARGET unofficial::mysql-connector-cpp::connector APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES "OpenSSL::SSL;OpenSSL::Crypto;resolv;dl"
  )
