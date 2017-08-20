cmake_minimum_required(VERSION 3.1)
project(agdg_server)

set(SANITIZER OFF CACHE BOOL "Enable -fsanitize=address")
set(STATIC_RUNTIME OFF CACHE BOOL "Link runtime libraries statically")
set(STATIC_V8 OFF CACHE BOOL "Link V8 statically")
set(SYSTEM_V8 OFF CACHE BOOL "Use system V8 rather than a custom build")
set(WITH_V8 OFF CACHE BOOL "Build with V8")
set(V8_INCLUDE "${PROJECT_SOURCE_DIR}/dependencies/v8/include" CACHE PATH "Path to V8 include/")
set(V8_LIB "${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release" CACHE PATH "Path to V8 out/<target>/")

set(BINARY agdgserverd)

set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE sources src/*.cpp src/*.hpp src/*.c src/*.h)

if (NOT WITH_V8)
    file(GLOB_RECURSE remove_sources src/v8scripting/*)
    list(REMOVE_ITEM sources ${remove_sources})
else()
    file(GLOB_RECURSE remove_sources src/scripting/*_dom.cpp)
    list(REMOVE_ITEM sources ${remove_sources})
endif()

add_executable(${BINARY} ${sources})

target_compile_definitions(${BINARY} PUBLIC
        ASIO_STANDALONE
        )

target_include_directories(${BINARY} PUBLIC src
    dependencies/asio/asio/include
    dependencies/rapidjson/include
    dependencies/reflector/include
    dependencies/websocketpp
)

if (SANITIZER)
    target_compile_options(${BINARY} PUBLIC -fsanitize=address)
    target_link_libraries(${BINARY} -fsanitize=address)
endif()

# Configure V8
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Any Clang/GCC/ICC (including mingw which is currently unsupported)

    target_compile_options(${BINARY} PUBLIC
            -Wall
            -Wextra
            -Werror=switch
            -Wno-unused-local-typedef
            -Wno-unused-parameter
            )

    if (NOT APPLE)
        if (STATIC_RUNTIME)
		    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -s")
        endif()

        target_link_libraries(${BINARY} pthread)
	endif()

    if (WITH_V8)
        if (APPLE)
            #file(GLOB_RECURSE V8_LIBS ${V8_LIB}/lib*.dylib)
            set(V8_LIBS ${V8_LIB}/libv8_libplatform.a ${V8_LIB}/libv8_libbase.a ${V8_LIB}/libv8.dylib)
        else()
            if (SYSTEM_V8)
                if (NOT STATIC_V8)
                    set(V8_LIBS
                            libv8.so
                            libv8_libplatform.a
                            )
                else()
                    set(V8_LIBS
                            libv8_base.a
                            libv8_external_snapshot.a
                            libv8_libbase.a
                            libv8_libplatform.a
                            libicui18n
                            libicuuc
                            libicudata
                            )
                endif()
            else()
                if (NOT STATIC_V8)
                    set(V8_LIBS
                            ${V8_LIB}/obj/libv8.so
                            ${V8_LIB}/obj/libv8_libplatform.a
                            ${V8_LIB}/obj/third_party/icu/libicui18n.so
                            ${V8_LIB}/obj/third_party/icu/libicuuc.so
                            )
                else()
                    set(V8_LIBS
                            ${V8_LIB}/obj/libv8_base.a
                            ${V8_LIB}/obj/libv8_external_snapshot.a
                            ${V8_LIB}/obj/libv8_libbase.a
                            ${V8_LIB}/obj/libv8_libplatform.a
                            ${V8_LIB}/obj/third_party/icu/libicui18n.a
                            ${V8_LIB}/obj/third_party/icu/libicuuc.a
                            ${V8_LIB}/obj/third_party/icu/libicudata.a
                            dl
                            rt
                            )
                endif()
            endif()
        endif()
    endif()
else()
    # Visual C++

    target_compile_definitions(${BINARY} PUBLIC
            _WEBSOCKETPP_CPP11_STL_
    )

    if (WITH_V8)
        if (SYSTEM_V8)
            error()
        elseif (NOT STATIC_V8)
            error()
        else()
            set(V8_LIBS
                    ${V8_LIB}/obj/third_party/icu/icui18n.lib
                    ${V8_LIB}/obj/third_party/icu/icuuc.lib
                    ${V8_LIB}/obj/v8_base_0.lib
                    ${V8_LIB}/obj/v8_base_1.lib
                    ${V8_LIB}/obj/v8_external_snapshot.lib
                    ${V8_LIB}/obj/v8_libbase.lib
                    ${V8_LIB}/obj/v8_libplatform.lib
                    ${V8_LIB}/obj/v8_libsampler.lib
                    )
        endif()
    endif()

    target_link_libraries(${BINARY} winmm dbghelp shlwapi)

    # V8 requires us to use static runtime
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

if (WITH_V8)
    target_compile_definitions(${BINARY} PUBLIC WITH_V8=1)
    target_include_directories(${BINARY} PUBLIC ${V8_INCLUDE})
    target_link_libraries(${BINARY} ${V8_LIBS})
endif()

add_custom_target(v8_blobs)
add_custom_command(TARGET v8_blobs PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${V8_LIB}/natives_blob.bin ${PROJECT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${V8_LIB}/snapshot_blob.bin ${PROJECT_SOURCE_DIR}
)

add_custom_target(run
    COMMAND ${BINARY}
    DEPENDS ${BINARY} v8_blobs
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
