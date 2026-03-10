cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# model_lib
add_library(model_lib STATIC
	src/collision_detector.h
	src/collision_detector.cpp
	src/data_keeper.h
	src/geom.h
	src/loot_generator.h
    src/loot_generator.cpp
	src/model.h
	src/model.cpp
	src/tagged.h
	src/utils.h
	src/utils.cpp
)

target_link_libraries(model_lib PUBLIC CONAN_PKG::boost Threads::Threads)

# game_server
add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/boost_logger.h
	src/app.h
	src/app.cpp
	src/program_options.h
	src/ticker.h
	src/model_serialization.h
	src/model_serialization.cpp
	src/connection_pool.h
	src/postgres/postgres.h
	src/postgres/postgres.cpp
)

target_link_libraries(game_server CONAN_PKG::boost Threads::Threads CONAN_PKG::libpqxx model_lib)