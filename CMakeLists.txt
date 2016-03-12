cmake_minimum_required(VERSION 3.1)
project(agdg_server)

set(WITH_V8 ON CACHE BOOL "Build with V8")
set(V8_INCLUDE "${PROJECT_SOURCE_DIR}/dependencies/v8/include" CACHE PATH "Path to V8 include/")
set(V8_LIB "${PROJECT_SOURCE_DIR}/dependencies/v8/out/x64.release" CACHE PATH "Path to V8 out/<target>/")

set(BINARY agdgserverd)

set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE sources src/*.cpp src/*.c)

if (NOT WITH_V8)
    file(GLOB_RECURSE remove_sources src/v8scripting/*)
    list(REMOVE_ITEM sources ${remove_sources})
endif()

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
		target_link_libraries(${BINARY} pthread)
	endif()

    if (WITH_V8)
        target_compile_options(${BINARY} PUBLIC
            -DWITH_V8=1
        )

        if (APPLE)
            #file(GLOB_RECURSE V8_LIBS ${V8_LIB}/lib*.dylib)
            set(V8_LIBS ${V8_LIB}/libv8_libplatform.a ${V8_LIB}/libv8_libbase.a ${V8_LIB}/libv8.dylib)
        else()
        endif()

        #file(GLOB_RECURSE V8_LIBS ${V8_LIB}/lib*.a)

        target_include_directories(${BINARY} PUBLIC ${V8_INCLUDE})
        target_link_libraries(${BINARY} ${V8_LIBS})
    endif()
endif()

add_custom_target(run
    COMMAND ${BINARY}
    DEPENDS ${BINARY}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
