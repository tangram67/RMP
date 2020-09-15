#!/bin/sh

LIB="mpg123-1.25.13"
TAR=$LIB.tar.bz2

if [ ! -f "$TAR" ]; then
  wget https://www.mpg123.de/download/$TAR
fi

rm -r $LIB
tar -xf $TAR

cd $LIB/
./configure --disable-shared --enable-int-quality && make

cp src/libmpg123/.libs/libmpg123.a ../lib$LIB.a
cp src/libmpg123/.libs/libmpg123.a ../libmpg123.a

