#!/bin/bash

unset  LSAN_OPTIONS
export LSAN_OPTIONS
set -eEuo pipefail
#set -x

D=/build/mount_squash_root_test2
EXEC=$1

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
#exit -1
mount --make-unbindable /  # needed on some systems where / is "shared", prevents EINVAL on mount --move

mount   | grep "$D" && exit -1
losetup | grep "$D" && exit -1


rm -rf $D
mkdir -p \
  $D/initramfs/dev  \
  $D/initramfs/proc \
  $D/initramfs/physical  \
  $D/initramfs/newroot  \
  $D/boot  \
  $D/squash1/sbin \
  $D/squash1/dev \
  $D/squash1/mnt/physical \


truncate -s 512M $D/boot.img
mkfs.vfat $D/boot.img > /dev/null

BOOT_DEV=$( losetup -f --show $D/boot.img )

mount $BOOT_DEV $D/boot

cat << EOF > $D/fake_init.c
#include <stdio.h>
int main () {
  printf("This is the fake init...\n");
}
EOF
clang $D/fake_init.c --static -o $D/squash1/sbin/init


/build/root-pi0w-dev-sdk/host/bin/mksquashfs $D/squash1/ $D/boot/squash1 -quiet -no-progress

for f in $( ldd $EXEC | sed -nE 's/[^\/]*(\/[^ ]*).*/\1/p'); do {
  mkdir -p $D/initramfs/$(dirname $f)
  cp $f $D/initramfs/$f
}; done

cp $EXEC $D/initramfs/init

mount -o bind /proc $D/initramfs/proc


chroot $D/initramfs/ /init $BOOT_DEV squash1
cleanup


