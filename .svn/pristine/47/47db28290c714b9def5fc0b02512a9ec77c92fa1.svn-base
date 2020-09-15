#!/bin/sh

if [ "$(id -u)" != "0" ]; then
  echo "This script must be run as root" 1>&2
  exit 1
fi

if [ ".$1" = "." ]; then
  echo "No file given." 1>&2
  exit 1
fi

if [ ! -f $1 ]; then
  echo "File does not exists." 1>&2
  exit 1
fi

# Set additional capabilities
setcap cap_ipc_lock,cap_net_bind_service,cap_sys_admin,cap_sys_nice=+ep $1

# Show capabilities
getcap $1
