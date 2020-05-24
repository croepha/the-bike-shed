echo "Not ready for general consumption, enter at your own risk"
exit

apt install -y cpio rsync sudo ccache

mkdir -p /build/root
cd /build/root

BR_VERSION=2020.02.2
wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-$BR_VERSION.tar.gz
tar xvf buildroot.tar.gz --strip-components=1


export FORCE_UNSAFE_CONFIGURE=1
export GIT_DIR=/workspaces/the-bike-shed/.git
export GIT_WORK_TREE=/workspaces/the-bike-shed/

function make() {
    /usr/bin/make -C /build/root O=/build/root$VARIANT "$@"
}

function full_build() {
    make clean
    make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/$VARIANT-root.config
    make all
}

function save_configs() {
    make savedefconfig
    make linux-update-defconfig
    make busybox-update-config
    make uclibc-update-config
}

function compress_release() {
    _i=/build/root$VARIANT/images/
    _o=/workspaces/the-bike-shed/build/
    mkdir -p $_o
    cp -v $_i/rootfs.squashfs $_o/$VARIANT-rootfs.squashfs
    cp -v $_i/sdcard.img      $_o/$VARIANT-sdcard.img
    xz --best --extreme -v $_o/$VARIANT-rootfs.squashfs
    xz --best --extreme -v $_o/$VARIANT-sdcard.img
}

VARIANT="pi0w-dev"
make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/$VARIANT-root.config
make linux-configure
make busybox-configure
make uclibc-configure

VARIANT="pi1-dev"
make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/$VARIANT-root.config
make linux-configure &&
make busybox-configure &&
make uclibc-configure

VARIANT="host-dev"
make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/$VARIANT-root.config
make linux-configure
make busybox-configure
make uclibc-configure

save_configs
make all

VARIANT="pi0w-dev"
VARIANT="pi1-dev"
XZ_OPT="-9e --threads=0 -v" tar cJvf $_o/$VARIANT-host.tar.xz -C /build/root$VARIANT/host .


# on mac
_o=/workspaces/the-bike-shed/build/
_b=~/the-bike-shed/build/
VARIANT="pi0w-dev"
# scp -v super1:$_o/$VARIANT-rootfs.squashfs.xz $_b
# scp -v super1:$_o/$VARIANT-sdcard.img.xz      $_b
scp -v super1:$_o/$VARIANT-host.tar.xz      $_b
# inside container:
rm -rvf /build/pi0w-dev-host/
mkdir -p /build/pi0w-dev-host/
tar xJvf pi0w-dev-host.tar.xz -C /build/pi0w-dev-host/
cp -v /build/pi0w-dev-host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/{crtbeginT.o,crtend.o} \
  /build/pi0w-dev-host/arm-buildroot-linux-uclibcgnueabihf/sysroot/usr/lib/


VARIANT="pi1-dev"
scp -v super1:$_o/$VARIANT-rootfs.squashfs.xz $_b
scp -v super1:$_o/$VARIANT-sdcard.img.xz      $_b

VARIANT="host-dev"
scp -v super1:$_o/$VARIANT-rootfs.squashfs.xz $_b
scp -v super1:$_o/$VARIANT-sdcard.img.xz      $_b



VARIANT="pi1-dev"
VARIANT="host-dev"
VARIANT="pi0w-dev"

make menuconfig
make linux-menuconfig
make busybox-menuconfig
make uclibc-menuconfig
save_configs
full_build


chown -R notroot /build /workspaces
sudo -u notroot bash
cd /build/root
make O=/build/root1/ raspberrypi_defconfig


make O=/build/root1/ menuconfig
make O=/build/root1/ savedefconfig

make O=/build/root1/ defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root1.config
make O=/build/root1/ make all

make O=/build/root1/ linux-menuconfig
make O=/build/root1/ linux-update-defconfig

make O=/build/root1/ busybox-menuconfig
make O=/build/root1/ busybox-update-config

make O=/build/root1/ uclibc-menuconfig
make O=/build/root1/ uclibc-update-config


make O=/build/root-dev/ defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root-dev.config
make O=/build/root-dev/ menuconfig
make O=/build/root-dev/ savedefconfig

make O=/build/root-dev/ busybox-menuconfig
make O=/build/root-dev/ busybox-update-config

make O=/build/root-dev/ uclibc-menuconfig
make O=/build/root-dev/ uclibc-update-config

make O=/build/root-dev/ all


exit




# Notes
# For a new buildroot project:
#  dl, extract
#  make defconfig <base>
#  make menuconfig ... update all the def config options
#  then do all the saveconfigs...

"
TODO:
 - Add curl/libcurl
 - Maybe switch to using IWD?
 - busybox dns?
 - allow dhcp
 - Add nano (from busybox???)
 - Set static IP
 - TODO output IP Address
 - Disable serial port in inittab
 - Remove logging daemon?
"


cat << EOF > physical/autoexec.sh
#!/bin/sh

ip link set dev eth0 up
ip addr add 192.168.4.30/24 dev eth0
ip route add default via 192.168.4.1

echo "nameserver 8.8.8.8" > /etc/resolv.conf
# TODO: wpa_cli -a ....


while true; do {
    echo -e "\n\n\n\n\n"
    ip a
    sleep 5
}; done

EOF
chmod +x physical/autoexec.sh


ln -sf /mnt/physical/wpa_supplicant.conf  etc/wpa_supplicant.conf

cat << EOF > physical/wpa_supplicant.conf
update_config=1
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1

network={
    key_mgmt=WPA-PSK
    ssid="test-ap2"
    psk="the-bike-shed"
}
EOF

cat << EOF > root/etc/inittab
::sysinit:/bin/mount -t proc proc /proc
::sysinit:/bin/mount -o remount,rw /
::sysinit:/bin/mkdir -p /dev/pts /dev/shm
::sysinit:/bin/mount -a
::sysinit:/sbin/swapon -a
null::sysinit:/bin/ln -sf /proc/self/fd /dev/fd
null::sysinit:/bin/ln -sf /proc/self/fd/0 /dev/stdin
null::sysinit:/bin/ln -sf /proc/self/fd/1 /dev/stdout
null::sysinit:/bin/ln -sf /proc/self/fd/2 /dev/stderr
::sysinit:/bin/hostname -F /etc/hostname
# now run any rc s$commonipts
::sysinit:/etc/init.d/rcS

# Put a getty on the serial port
console::respawn:/sbin/getty -L  console 0 vt100 # GENERIC_SERIAL
tty1::respawn:sh /mnt/physical/autoexec.sh
tty2::respawn:/sbin/getty -L  tty2 0 vt100 # HDMI console

# Stuff to do for the 3-finger salute
#::ctrlaltdel:/sbin/reboot

# Stuff to do before rebooting
::shutdown:/etc/init.d/rcK
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r
EOF


