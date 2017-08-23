FROM fedora:26
WORKDIR /srv/agdg-server

COPY dependencies dependencies
COPY scripts scripts
COPY src src
COPY CMakeLists.txt ./

RUN dnf -y makecache &&\
        dnf -y install cmake gcc gcc-c++ libicu libicu-devel libstdc++ make v8-devel &&\
        cmake -DWITH_V8=1 -DSTATIC_V8=1 -DSYSTEM_V8=1 -DV8_LIB=/usr/lib64 . &&\
        make agdgserverd v8_blobs &&\
        dnf -y remove cmake gcc gcc-c++ libicu-devel make v8-devel &&\
        dnf clean all &&\
        rm -rf dependencies src CMakeLists.txt

ENTRYPOINT ./agdgserverd
EXPOSE 80 9001 9002 9003
