set -eEuo pipefail
set -x

losetup -D

mkdir -p /build/dev-root-test2/
cd /build/dev-root-test2/
umount proc/  || true
umount dev/   || true
rm -rf /build/dev-root-test2/*
cd /build/dev-root-test2/

mkdir -p lib64/ lib/ proc/ usr/bin dev/
mount -t proc none proc/
mount -t devtmpfs dev2 dev/
! findmnt dev/
cp /workspaces/the-bike-shed/build/root-dev.squashfs /build/mount_squash_root.exec .

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

# lldb-server-10 g :5001 --  \
exec \
  /usr/sbin/chroot . ./mount_squash_root.exec


#code --reuse-window --file-uri 'vscode://vadimcn.vscode-lldb/launch/config?{"type": "lldb","request": "custom","name": "attach 5001","targetCreateCommands": ["target create /usr/sbin/chroot"],"processCreateCommands": ["gdb-remote 127.0.0.1:5001"],}'

