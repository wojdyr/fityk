Building Fityk for OS X


0. Make sure you have the Mac OS 10.4 SDK installed.

1. Build wxMac 2.8 and install it somewhere
    $ ./configure --prefix=/path/to/wxMac --enable-monolithic \
        --disable-shared --disable-debug --enable-universal_binary
    $ make install

2. Build fityk
    $ env \
        CFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386" \
        CXXFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386" \
        ./configure --with-wx-prefix=/path/to/wxMac --without-readline \
        --disable-python --disable-shared --enable-static \
        --disable-dependency-tracking
    $ make

3. Build and test Fityk.app
    $ cd macosx
    $ make
    $ open Fityk.app
