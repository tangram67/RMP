#!/bin/sh

rm -r curl
git clone https://github.com/curl/curl.git

cd curl

if [ "$1" = "disable-https" ]; then
  echo "HTTPS support disabled"
  export LDFLAGS='-static'
  ./buildconf && ./configure --disable-shared
else
  echo "Build with HTTPS support"
  ./buildconf && ./configure --disable-shared --with-ssl
fi

# GIT ships it's own libtool, overwrite with systems one
cp /usr/bin/libtool .
make

cp include/curl/*.h ..
cp lib/.libs/libcurl.a ..

