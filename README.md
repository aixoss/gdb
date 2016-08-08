Building 64-bit gdb on AIX
==========================

gdb compiles with gcc on AIX. 
To build gdb on AIX with gcc follow the below instructions.


Environment variables

    export CC=gcc
    export CXX=g++
    export OBJECT_MODE=64
    export CFLAGS="-maix64 -D_LARGE_FILES -g"
    export LDFLAGS="-L/usr/lib -L/opt/freeware/lib -blibpath:/usr/lib:/lib:/opt/freeware/lib: -ldemangle" 

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

NOTE: For using aix demangling api we need to do couple of things before the compilation.

1. Copy the /usr/include/demangle.h file to gdb build path. something like to gdb-7.9.1/include.
   For example "cp /usr/include/demangle.h gdb-7.9.1/include"
2. Have the /usr/lib/libdemangle.a library for using AIX demangle API.
   This library is part of AIX xlC compiler.

And -ldemangle linker option is provided through LDFLAGS.
