#!/bin/bash

unset  LSAN_OPTIONS
export LSAN_OPTIONS
set -eEuo pipefail
# set -x

D=/build/mount_squash_root_test2
EXEC=/build/mount_squash_root.dbg.exec

function cleanup() {
{
mkdir -p $D/tmprm0
mount --move $D/initramfs/mnt/physical $D/tmprm0 || true
losetup -D
umount -R   $D/initramfs/ $D/tmprm0  $D/* || true
umount -R   $D/initramfs/* || true
} &> /dev/null
}

cleanup

mount   | grep "$D" && exit -1
losetup | grep "$D" && exit -1


rm -rf $D
mkdir -p \
  $D/initramfs/dev  \
  $D/initramfs/proc \
  $D/initramfs/physical  \
  $D/initramfs/newroot  \
  $D/boot  \
  $D/squash1 \
  $D/squash1/dev \
  $D/squash1/mnt/physical \


truncate -s 512M $D/boot.img
mkfs.vfat $D/boot.img > /dev/null

BOOT_DEV=$( losetup -f --show $D/boot.img )

mount $BOOT_DEV $D/boot

cp /usr/bin/busybox $D/squash1/

/build/rootpi0w-dev/host/bin/mksquashfs $D/squash1/ $D/boot/squash1 -quiet -no-progress

for f in $(ldd $EXEC | sed -nE 's/[^\/]*(\/[^ ]*).*/\1/p'); do {
  mkdir -p $D/initramfs/$(dirname $f)
  cp $f $D/initramfs/$f
}; done

cp $EXEC $D/initramfs/init1.exec

# mount -t devtmpfs none $D/initramfs/dev
mount -o bind /proc $D/initramfs/proc
# mount -o bind /sys  $D/initramfs/sys

chroot $D/initramfs/ /init1.exec $BOOT_DEV squash1
cleanup


