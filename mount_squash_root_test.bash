set -eEuo pipefail

cd /build/dev-root-test2/ || true
umount proc/  || true
rm -rf /build/dev-root-test2/
mkdir /build/dev-root-test2/
cd /build/dev-root-test2/

mkdir -p lib64/ lib/ proc/ usr/bin
cp /workspaces/the-bike-shed/build/root-dev.squashfs /build/mount_squash_root.exec .
mount -t proc none proc/

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

#strace
chroot . ./mount_squash_root.exec
