set -eEuo pipefail
set -x

SDCARD_DEV=/dev/loop3

mkdir -p /tmp/physical
mount --move /build/dev-root-test2/mnt/physical/ /tmp/physical || true
umount -R /build/dev-root-test2 || true
umount /dev/loop0  || true
losetup -d /dev/loop0 || true
umount -R /build/dev-root-test2 || true
umount /dev/loop3  || true
losetup -d /dev/loop3 || true
losetup -D

# Create a simulation of the physical root (cleanup from last run):
umount /build/dev-sdcard-tmp/ || true
losetup -d $SDCARD_DEV || true
# Create a simulation of the physical root (The SDCard...):
truncate -s 512M /build/dev-sdcard.img
mkfs.vfat /build/dev-sdcard.img
mkdir -p /build/dev-sdcard-tmp/
losetup $SDCARD_DEV /build/dev-sdcard.img
mount $SDCARD_DEV /build/dev-sdcard-tmp/
cd /build/dev-sdcard-tmp/
cp /workspaces/the-bike-shed/build/root-dev.squashfs .
cd /
umount /build/dev-sdcard-tmp/
# at this point $SDCARD_DEV is our simulated sdcard

# Simulate a pre-mounted initramfs (cleanup from last run):
mkdir -p /build/dev-root-test2/
cd /build/dev-root-test2/
umount proc/  || true
umount dev/   || true
cd /
umount /build/dev-root-test2/ || true
# Simulate a pre-mounted initramfs
mount -t tmpfs none /build/dev-root-test2/
cd /build/dev-root-test2/
mkdir -p lib64/ lib/ proc/ usr/bin dev/ physical/ newroot/
mount -t proc none proc/
mount -t devtmpfs dev2 dev/
cp /build/mount_squash_root.exec .
cp /usr/bin/llvm-symbolizer-10 usr/bin
cp \
  /lib/x86_64-linux-gnu/libLLVM-10.so.1 \
  /lib/x86_64-linux-gnu/libbsd.so.0 \
  /lib/x86_64-linux-gnu/libc.so.6 \
  /lib/x86_64-linux-gnu/libdl.so.2 \
  /lib/x86_64-linux-gnu/libedit.so.2 \
  /lib/x86_64-linux-gnu/libffi.so.7 \
  /lib/x86_64-linux-gnu/libgcc_s.so.1 \
  /lib/x86_64-linux-gnu/libm.so.6 \
  /lib/x86_64-linux-gnu/libpthread.so.0 \
  /lib/x86_64-linux-gnu/librt.so.1 \
  /lib/x86_64-linux-gnu/libstdc++.so.6 \
  /lib/x86_64-linux-gnu/libtinfo.so.6 \
  /lib/x86_64-linux-gnu/libz.so.1 \
  lib/
cp \
  /lib64/ld-linux-x86-64.so.2 \
  lib64/


# Cleanup from last run....
losetup -d /dev/loop0 || true
#strace
# lldb-server-10 g :5001 --  \
exec \
  /usr/sbin/chroot . ./mount_squash_root.exec vfat $SDCARD_DEV /root-dev.squashfs


#code --reuse-window --file-uri 'vscode://vadimcn.vscode-lldb/launch/config?{"type": "lldb","request": "custom","name": "attach 5001","targetCreateCommands": ["target create /usr/sbin/chroot"],"processCreateCommands": ["gdb-remote 127.0.0.1:5001"],}'

