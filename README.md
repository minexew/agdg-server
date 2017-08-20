[![Travis Build](https://travis-ci.org/minexew/agdg-server.svg?branch=master)](https://travis-ci.org/minexew/agdg-server)

Introduction
============

On some operating systems, such as Fedora 26, you can download pre-built V8 using your package manager. This is recommended.
Otherwise, you'll need to choose between a static and dynamic build (static is less hassle, but the application binary will take longer to link).

Building V8 is always a clusterfuck. Don't get discouraged if you keep getting obscure errors from wacky Google tools. It's normal.
If stuck, you can refer to the travis config which targets a pristine enviroment with no pre-installed tools.

Fetching V8
==========
```
cd dependencies

# depot tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"

# clone v8 repository
fetch v8
cd v8

# pick a known-good version
git checkout tags/6.0.286.55
gclient sync
```

On Windows, you'll need to use CMD, not bash. Also don't forget to `set DEPOT_TOOLS_WIN_TOOLCHAIN=0` first.)

Bulding V8
==========
You'll need to have Python in your `$PATH`. On Windows, you can use the one from `depot_tools`.

As of 2017/08, V8's build system is fucked again, and if you want a static build, you'll need to edit `v8/gni/v8.gni` to `v8_static_library = true`.

## Mac
TODO: update 2017
```
# at time of writing (2016/04) V8 will default to libstdc++
# force it to use libc++ instead which we need for C++14 support
export CXX="`which clang++` -std=c++11 -stdlib=libc++"
export LINK="`which clang++` -std=c++11 -stdlib=libc++"
export GYP_DEFINES="clang=1 mac_deployment_target=10.8"
make -j4 library=shared x64.release
```
## Linux

```
gn gen out.gn/x64.release --args='is_debug=false target_cpu="x64" is_component_build=false v8_static_library=true'
ninja -j 4 -C out.gn/x64.release
```

## Windows
Visual Studio 2015 + Debugging Tools for Windows is currently required by V8. The codebase actually supports 2017 now, but `v8/gypfiles/vs_toolchain.py` must be manually updated by copying from Chromium repo.

```
gn gen out.gn/x64.optdebug --args='target_cpu="x64" is_component_build=false v8_static_library=true'
ninja -C out.gn/x64.optdebug
```

Building V8 in Debug mode is of course possible as well. Do note, however, that a Debug build of V8 will take a few gigabytes on disk.

Of course, if you don't need debugging, you can go for a Release build. This precludes debugging of not only V8, but also the application itself.

Building agdg-server
====================

This is ridiculously easy in comparison to messing with V8. The options will vary based on your setup.

```
git submodule update --init --recursive

cmake -DWITH_V8=1 -DSTATIC_V8=1 -DSYSTEM_V8=0 .
make -j4
./agdgserverd
```
