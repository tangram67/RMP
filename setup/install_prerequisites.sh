#!/bin/sh

if [ "$(id -u)" != "0" ]; then
  echo "This script must be run as root" 1>&2
  exit 1
fi

apt-get install libgnutls28-dev libgcrypt20-dev \
  libssl-dev libjpeg-dev libasound2-dev \
  libpng-dev libexif-dev libcurl4-gnutls-dev \
  libflac-dev libiberty-dev binutils-dev \
  libpng-dev libudev-dev libcap-dev \
  binutils-dev cifs-utils \
  autoconf libtool cmake
#  libpng12-dev \
#  libpq-dev

