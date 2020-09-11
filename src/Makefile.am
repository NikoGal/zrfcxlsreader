AM_CPPFLAGS = -std=c++0x -DNDEBUG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DSAPonUNIX -DSAPwithUNICODE \
        -D__NO_MATH_INLINES -DSAPwithTHREADS -DSAPonLIN \
	-I/usr/sap/nwrfcsdk/include \
        /usr/local/include/libxls \
        /usr/local/include

AM_CXXFLAGS = -std=c++0x -O2 -minline-all-stringops -g -fno-strict-aliasing -fno-omit-frame-pointer -m64 \
        -fexceptions -funsigned-char -Wall -Wno-uninitialized -Wno-long-long -Wcast-align -pthread -fPIC

AM_LDFLAGS = -std=c++0x -O2 -minline-all-stringops -g -fno-strict-aliasing -fno-omit-frame-pointer  -m64 \
        -fexceptions -funsigned-char -Wall -Wno-uninitialized -Wno-long-long -Wcast-align -pthread -fPIC

bin_PROGRAMS = zrfcxlsreader
zrfcxlsreader_SOURCES = zrfcxlsreader.cpp

zrfcxlsreader_LDADD = -L/usr/sap/nwrfcsdk/lib -lsapucum -lsapnwrfc /usr/local/lib64/libxlsreader.la
