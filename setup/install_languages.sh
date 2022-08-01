#!/bin/sh

if [ "$(id -u)" != "0" ]; then
  echo "This script must be run as root" 1>&2
  exit 1
fi

locale-gen fr_FR.UTF-8
locale-gen it_IT.UTF-8
locale-gen es_ES.UTF-8
locale-gen pl_PL.UTF-8
locale-gen zh_CN.UTF-8

