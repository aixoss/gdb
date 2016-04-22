Building 64-bit gdb on AIX
==========================

gdb compiles with gcc on AIX. 
To build gdb on AIX with gcc follow the below instructions.


Environment variables

    export CC=gcc
    export CXX=g++
    export OBJECT_MODE=64
    export CFLAGS="-maix64 -D_LARGE_FILES -g"

Run configure

    ./configure --prefix=/opt/freeware \
                --infodir=/opt/freeware/info \
                --mandir=/opt/freeware/man \
                --disable-werror \
                --enable-sim \
                --target=powerpc64-ibm-aix6.1.2.0 \
                --build=powerpc64-ibm-aix6.1.2.0

To build the binaries

    gmake
