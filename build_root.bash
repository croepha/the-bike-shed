
# Our operating system is a distribution of GNU/Linux facilitated by buildroot

# The reason why we are using buildroot instead of just simply using
#  <insert your favorite distro here> is because for reliability we really want to limit
# the amount of software running on the device, as each additional peice of software
# competes for system resources, and often, they also increase the attack surface.
# But most importaintly, more pieces means more things to break.  Buildroot provides
# a very easy way to build custom OS images with only the things we want in them.
# Also it allows for some customizations, like for example, we put the whole OS in
# a squashfs image that sits on the root of the SD card, when we want to update,
# we just copy over a new squashfs image, no fiddling around with partitions and
# package managers.  Also all of the importaint configuration options are surfaced
# to the root of the SD Card, that way the user doesn't have to dig into deep directory
# trees to configure basic things like network settings.  See mount_squash_root.c for
# more detains

# Before you get started here, you should know that it may take a decent machine hours
# to do a full build, unless you are needing to reconfigure the base system, like for
# example, installing a text editor or network utility, or maybe installing a new
# system library, then you should probably just use the builds from the latest release
# there are also a number of files that from the buildroot build that are needed to
# build binaries, those can be found in the sdk.tar.xz file, the setup_dev_environment.sh
# script will download and extract that sdk

# A full explanation of how to use buildroot, is a bit out-of-scope, but please study
# the official buildroot manual: https://buildroot.org/downloads/manual/manual.html
# With all that said, here are some well defined recipes:
#  get_buildroot # Download and extract buildroot
#


export FORCE_UNSAFE_CONFIGURE=1
export XZ_OPT="-9e --threads=0 -v"
export _F="set -eEuo pipefail"
export BR_VERSION=2020.02.2
export HW_NAME=pi0w-dev
export SRC=/workspaces/the-bike-shed/
export BR=/build/root-$BR_VERSION
export OUT=root


function _make() ($_F
    /usr/bin/make -C $BR O=/build/$OUT-$HW_NAME "$@"
)

function _load_config() ($_F
    _make defconfig BR2_DEFCONFIG=$SRC/$HW_NAME-root.config
)

function get_buildroot() ($_F
    mkdir -p $BR
    cd $BR
    wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-$BR_VERSION.tar.gz
    tar xvf buildroot.tar.gz --strip-components=1
)

function menuconfig_root() ($_F
    _load_config
    _make menuconfig
    _make savedefconfig
)

function menuconfig_linux() ($_F
    _load_config
    _make linux-menuconfig
    _make linux-update-defconfig
)

function menuconfig_busybox() ($_F
    _load_config
    _make busybox-menuconfig
    _make busybox-update-config
)

function menuconfig_uclibc() ($_F
    _load_config
    _make uclibc-menuconfig
    _make uclibc-update-config
)

function clean_all() ($_F
    _load_config
    _make clean
)

function build_all() ($_F
    mkdir -p /build/pi0initramfs
    _load_config
    _make
)

function build_linux() ($_F
    rm -rvf  /build/pi0initramfs
    mkdir -p /build/pi0initramfs/{dev,physical,newroot}
    cp /build/mount_squash_root.staticpi0wdbg.exec /build/pi0initramfs/init
    /build/$OUT-$HW_NAME/host/arm-buildroot-linux-uclibcgnueabihf/bin/strip /build/pi0initramfs/init
    _load_config
    _make linux-rebuild
    # Copy /build/rootpi0w-dev/images/zImage to sdcard
)

function package_sdk() ($_F
    o1=/build/$OUT-$HW_NAME
    o2=$o1-sdk
    sr_src=$o1/host/arm-buildroot-linux-uclibcgnueabihf/sysroot/
    sr_dst=$o2/host/arm-buildroot-linux-uclibcgnueabihf/sysroot/

    function copy() {
        mkdir -p $o2/$1
        echo rsync -rvha $o1/$1/ $o2/$1/
        rsync -rvha $o1/$1/ $o2/$1/
    }

    rm -rvf $o2

    mkdir -p $sr_dst/usr/lib/ $sr_dst/lib/

    copy host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/
    copy host/libexec/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/
    copy host/arm-buildroot-linux-uclibcgnueabihf/bin/
    copy host/arm-buildroot-linux-uclibcgnueabihf/sysroot/usr/include/
    copy host/bin/

    rsync -vha \
        $sr_src/usr/lib/crt*.o \
        $sr_src/usr/lib/libpthread*.a \
        $sr_src/usr/lib/libutil.* \
        $sr_src/usr/lib/libc.* \
        $sr_src/usr/lib/libgcc_s.so* \
        $sr_src/usr/lib/libcurl.so* \
        $sr_src/usr/lib/uclibc_nonshared.a \
        $sr_src/usr/lib/libssl.so* \
        $sr_src/usr/lib/libcrypto.so* \
        $sr_src/usr/lib/libz.so* \
        $sr_src/usr/lib/libatomic.so* \
        $sr_dst/usr/lib/

    rsync -vha \
        $sr_src/lib/libc.so.1 \
        $sr_src/lib/libuClibc-1.0.32.so \
        $sr_src/lib/ld-uClibc-1.0.32.so \
        $sr_src/lib/ld-uClibc.so.1 \
        $sr_dst/lib/

    rm -rvf $o2/host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/install-tools/
    rm -rvf $o2/host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/plugin

    tar cJvf $o2.tar.xz -C $o2 .
    echo "Made release: $o2.tar.xz"

)

eval "$@"



exit

# Everything below here is junk notes that need to be removed or moved elsewhere:
#--------------------------------------------


# recipe full build:
# bash build_root2.bash _load_config
# bash build_root2.bash _make clean
# bash build_root2.bash _make

# recipe root menuconfig:
# bash build_root2.bash _load_config
# bash build_root2.bash _make menuconfig
# bash build_root2.bash _make savedefconfig

# recipe uclibc menuconfig:

# make busybox-menuconfig
# make busybox-update-config

# To make SD Card image:
# copy /build/root-pi0w-dev/images/rpi-firmware/*
# override config files





wpa_supplicant -B  -i wlan0 -c /mnt/physical/wpa_supplicant.conf
wpa_cli -Ba /mnt/physical/wifi_action.sh


# cat /mnt/physical/wifi_action.sh
#!/bin/sh
#echo WIFI ACTION RUNNING $1 $2 >> /tmp/wifi_action.log

if [ "$2" = "CONNECTED" ]; then
#    ip link set dev wlan0 up
    ip addr add 192.168.4.31/24 dev wlan0
    ip route add default via 192.168.4.1
    echo "nameserver 8.8.8.8" > /etc/resolv.conf
fi




eval "$@"

exit 0
while true; do {
    read
    echo "pausing"
    sleep 2
}; done


echo "Not ready for general consumption, enter at your own risk"
exit -1

function setup() ($_F
    echo "Not ready for general consumption, enter at your own risk"
    exit -1

    apt install -y cpio rsync sudo ccache

    mkdir -p /build/root
    cd /build/root

    BR_VERSION=2020.02.2
    wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-$BR_VERSION.tar.gz
    tar xvf buildroot.tar.gz --strip-components=1

)


VARIANT="pi0w-dev" bash build_root.bash build_remote clean
VARIANT="pi1-dev" bash build_root.bash build_remote clean
VARIANT="host-dev" bash build_root.bash build_remote clean


VARIANT="pi0w-dev"
save_configs  &
VARIANT="pi1-dev"
save_configs &
VARIANT="pi0w-dev"
save_configs &


_w=/workspaces/the-bike-shed/
_o=$_w/build/






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
make linux-menuconfig
make menuconfig


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



losetup -D
losetup -f --show -P /workspaces/the-bike-shed/build/pi0w-dev-sdcard.img
mount /dev/loop0p2 /mnt
cd /mnt

# TODO: We should make sure that iptables is installed, and we should block everything

cat << EOF > autoexec.sh
#!/bin/sh

date --set "$( date -r /boot/shed-config '+%F %T' )"

modprobe pwm-bcm2835
/root/supervisor.pi0wdbg.exec /root/supervisor.config &

/usr/sbin/telnetd -l sh

hostname shed-unconfigured
modprobe brcmfmac
dd if=/dev/hwrng of=/dev/random bs=2048 count=16 &
ip link set dev wlan0 up
ip addr add 192.168.4.31/24 dev wlan0
ip route add default via 192.168.4.1
echo "nameserver 8.8.8.8" > /etc/resolv.conf
wpa_supplicant -B  -i wlan0 -c /boot/wpa_supplicant.conf
# TODO: Set date to mod time of the config file
ntpd -p 0.us.pool.ntp.org -p 1.us.pool.ntp.org -p 2.us.pool.ntp.org -p 3.us.pool.ntp.org
# TODO: Should add auth keys for ntp

# # TODO: wpa_cli -a ....

while true; do {
    echo -e "\n\n\n\n\n"
    ip link
    echo
    ip addr
    sleep 5
}; done
EOF

cat << EOF > wpa_supplicant.conf
update_config=1
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1

network={
    key_mgmt=WPA-PSK
    ssid="test-ap2"
    psk="the-bike-shed"
}
EOF

cat << EOF > cmdline.txt
root=/dev/mmcblk0p2 rootwait console=tty1
EOF

cat << EOF > config.txt
kernel=zImage
disable_overscan=1
gpu_mem_256=50
gpu_mem_512=50
gpu_mem_1024=50
dtoverlay=miniuart-bt
dtoverlay=pwm-2chan
EOF

cat << EOF > etc/inittab
::sysinit:/bin/mount -t proc proc /proc
::sysinit:/bin/mount -o remount,rw /
::sysinit:/bin/mkdir -p /dev/pts /dev/shm
::sysinit:/bin/mount -a
::sysinit:/sbin/swapon -a
null::sysinit:/bin/ln -sf /proc/self/fd /dev/fd
null::sysinit:/bin/ln -sf /proc/self/fd/0 /dev/stdin
null::sysinit:/bin/ln -sf /proc/self/fd/1 /dev/stdout
null::sysinit:/bin/ln -sf /proc/self/fd/2 /dev/stderr
::sysinit:/sbin/modprobe brcmfmac
::sysinit:/bin/hostname -F /etc/hostname
# now run any rc s$commonipts
::sysinit:/etc/init.d/rcS

# Put a getty on the serial port
# remove console::respawn:/sbin/getty -L  console 0 vt100 # GENERIC_SERIAL
tty1::respawn:sh /autoexec.sh
tty2::respawn:/sbin/getty -L  tty2 0 vt100 # HDMI console

# Stuff to do for the 3-finger salute
#::ctrlaltdel:/sbin/reboot

# Stuff to do before rebooting
::shutdown:/etc/init.d/rcK
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r
EOF





cd
umount /mnt
losetup -D

# From mac:
sudo dd if=/Users/streaming/the-bike-shed/build/pi0w-dev-sdcard.img \
  of=/dev/disk2 bs=4098





gdb notes:
gdbserver 0.0.0.0:6969 ./shed3.pi0wdbg.exec
gdb-multiarch
set sysroot /build/rootpi0w-dev/staging/
file /build/shed3.pi0wdbg.exec
target remote 192.168.4.31:1234
