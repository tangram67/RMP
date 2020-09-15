#/bin/sh

chown -R root:gpio /sys/class/gpio
chmod 0220 /sys/class/gpio/export
chmod 0220 /sys/class/gpio/unexport
