cmake_minimum_required(VERSION 3.1)
project(agdg_server)

set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE sources src/*.cpp src/*.c)

add_executable(agdgserverd ${sources})

target_compile_options(agdgserverd PUBLIC
    -Wall
    -DASIO_STANDALONE
)

target_include_directories(agdgserverd PUBLIC src
    dependencies/asio/asio/include
    dependencies/rapidjson/include
    dependencies/reflector/include
    dependencies/websocketpp
)

if(LINUX)
	target_link_libraries(agdgserverd pthread)
endif()
