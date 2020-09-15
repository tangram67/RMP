#!/bin/sh

NAME="rmp"
INIT="/etc/init.d/$NAME"
DEFAULTS="/etc/default/$NAME"
ARCHIVE="$NAME.update.tar.gz"

if [ ! -f $DEFAULTS ]; then
  echo "Defaults file <$DEFAULTS> does not exists." 1>&2
  exit 1
fi

# Read configuration variable file if it is present
[ -r $DEFAULTS ] && . $DEFAULTS

if [ ! -x $DAEMON ]; then
  echo "Daemon file <$DAEMON> does not exists." 1>&2
  exit 1
fi

CONF_DIR=$(dirname $HMP_CONF)
EXE_DIR=$(dirname $DAEMON)
WEBSERVER_CONF="$CONF_DIR/webserver.conf"
WEBSERVER_ROOT=$(sed -n 's/^[[:space:]]*DocumentRoot[[:space:]]*=[[:space:]]"\?\([^"]*\)\"\?/\1/p' $WEBSERVER_CONF)

if [ ! -d $WEBSERVER_ROOT ]; then
  echo "Webserver root folder <$WEBSERVER_ROOT> does not exists." 1>&2
  exit 1
fi

TAR_BIN_ROOT="/tmp/$NAME.tmp"
TAR_WEB_ROOT="$TAR_BIN_ROOT/wwwroot"

rm -rf $TAR_BIN_ROOT
mkdir $TAR_BIN_ROOT
mkdir $TAR_WEB_ROOT

cp $INIT $TAR_BIN_ROOT/
cp $DAEMON $TAR_BIN_ROOT/
cp -r $WEBSERVER_ROOT/* $TAR_WEB_ROOT/

# Create tar gzip archive
[ -f $ARCHIVE ] && rm $ARCHIVE
tar -czf $ARCHIVE -C $TAR_BIN_ROOT .

# Cleanup everything...
rm -rf $TAR_BIN_ROOT

