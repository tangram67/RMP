#!/bin/sh

if [ "$(id -u)" != "0" ]; then
  echo "This script must be run as root" 1>&2
  exit 1
fi

if [ ".$1" = "." ]; then
  writelog "Please specify a valid user name."
  exit 1
fi  

mkdir /etc/dbApps/
mkdir /var/log/dbApps
mkdir /usr/local/dbApps/

chown $1:$1 /etc/dbApps/
chown $1:$1 /var/log/dbApps
chown $1:$1 /usr/local/dbApps/

