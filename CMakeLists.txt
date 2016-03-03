cmake_minimum_required(VERSION 3.1)
project(agdg_server)

set(BINARY agdgserverd)

set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE sources src/*.cpp src/*.c)

add_executable(${BINARY} ${sources})

target_compile_options(${BINARY} PUBLIC
    -Wall
    -DASIO_STANDALONE
)

target_include_directories(${BINARY} PUBLIC src
    dependencies/asio/asio/include
    dependencies/rapidjson/include
    dependencies/reflector/include
    dependencies/websocketpp
)

if(UNIX)
	if (NOT APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -s")
		target_link_libraries($BINARY pthread)
	endif()
endif()

add_custom_target(run
    COMMAND ${BINARY}
    DEPENDS ${BINARY}
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
