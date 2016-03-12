![Travis Build](https://travis-ci.org/minexew/agdg-server.svg?branch=master)

Bulding on Mac
==============

```
# depot tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"

# clone v8 repository
fetch v8
cd v8

# pick a known-good version
git checkout tags/5.1.84
gclient sync

export CXX="`which clang++` -std=c++11 -stdlib=libc++"
export LINK="`which clang++` -std=c++11 -stdlib=libc++"
export GYP_DEFINES="clang=1 mac_deployment_target=10.8"
make -j4 library=shared x64.release
```