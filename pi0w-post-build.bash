#!/bin/bash

set -u
set -e

rm -f ${TARGET_DIR}/etc/init.d/S60openvpn
rm -f ${TARGET_DIR}/etc/init.d/S50telnet

cat << EOF > ${TARGET_DIR}/etc/inittab
::sysinit:/bin/mount -t proc proc /proc
::sysinit:/bin/mount -o remount,rw /
::sysinit:/bin/mkdir -p /dev/pts /dev/shm
::sysinit:/bin/mount -a
::sysinit:/sbin/swapon -a
null::sysinit:/bin/ln -sf /proc/self/fd /dev/fd
null::sysinit:/bin/ln -sf /proc/self/fd/0 /dev/stdin
null::sysinit:/bin/ln -sf /proc/self/fd/1 /dev/stdout
null::sysinit:/bin/ln -sf /proc/self/fd/2 /dev/stderr
::sysinit:/etc/init.d/rcS

# Put a getty on the serial port
# remove console::respawn:/sbin/getty -L  console 0 vt100 # GENERIC_SERIAL
tty1::respawn:/bin/sh /mnt/physical/autoexec.sh
tty2::respawn:/bin/sh
tty3::respawn:/sbin/getty -L  tty3 0 vt100

# Stuff to do for the 3-finger salute
#::ctrlaltdel:/sbin/reboot

# Stuff to do before rebooting
::shutdown:/etc/init.d/rcK
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r
EOF
