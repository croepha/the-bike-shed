export FORCE_UNSAFE_CONFIGURE=1
export GIT_WORK_TREE=/workspaces/the-bike-shed/
export GIT_DIR=$GIT_WORK_TREE/.git
export XZ_OPT="-9e --threads=0 -v"
export _F="set -eEuo pipefail"
export BR_VERSION=2020.02.2

function make() ($_F
    /usr/bin/make -C /build/root O=/build/root$VARIANT "$@"
)

function full_build() ($_F
    apt install -y cpio rsync sudo ccache build-essential unzip bc
    locale-gen en_US.UTF-8
    update-locale


    if [ ! -d /build/root ]; then
        mkdir -p /build/root
        cd /build/root
        wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-$BR_VERSION.tar.gz
        tar xvf buildroot.tar.gz --strip-components=1
    fi
    cd /build/root

    _b=/build/root$VARIANT/
    _w=/workspaces/the-bike-shed/
    _i=$_b/images/
    _o=$_w/build/

    if [[ "( "$@" )" =~ " clean " || ! -f "/build/root/$VARIANT/.config" ]]; then
        make clean
        rm -vf                    $_o/$VARIANT-*
    fi
    make defconfig BR2_DEFCONFIG=$_w/$VARIANT-root.config

    make all
    mkdir -p                  $_o
    cp -v $_i/rootfs.squashfs $_o/$VARIANT-rootfs.squashfs
    xz                        $_o/$VARIANT-rootfs.squashfs
    [[ ! "$VARIANT" =~ "host" ]] && {
        cp -v $_i/sdcard.img      $_o/$VARIANT-sdcard.img
        xz                        $_o/$VARIANT-sdcard.img
    }
    tar cJv -C $_b/host    . -f  $_o/$VARIANT-host.tar.xz
    tar cJv -C $_b/staging . -f  $_o/$VARIANT-staging.tar.xz
    rm -f /build/$VARIANT-full_build.working
)

function save_configs() ($_F
    make savedefconfig
    make linux-update-defconfig
    make busybox-update-config
    make uclibc-update-config
)

function archive() ( $_F
    _w=/workspaces/the-bike-shed/
    tar zc $_w/build_root.bash $_w/$VARIANT-*.config
)


function build_remote2() ( $_F
    SSH_HOST=build
    _w=/workspaces/the-bike-shed/
    archive | ssh $SSH_HOST tar zx -C /
    ssh "$SSH_HOST" 'touch /build/'$VARIANT'-full_build.working'
    ssh "$SSH_HOST" tmux new-session -Ad
    ssh "$SSH_HOST" build tmux set-option -g remain-on-exit on
    ssh "$SSH_HOST" tmux new-window -d \
        'bash '$_w'/build_root.bash VARIANT='$VARIANT' full_build'

)

function build_remote() ($_F
    SSH_HOST=build
    _w=/workspaces/the-bike-shed/
    _o=$_w/build/

    ssh "$SSH_HOST" 'touch /build/'$VARIANT'-full_build.working'
    ssh "$SSH_HOST" tmux new-session -Ad
    ssh "$SSH_HOST" build tmux set-option -g remain-on-exit on

    scp $_w/build_root.bash "$SSH_HOST":/build/
    ssh "$SSH_HOST" tmux new-window -d \
        'bash /build/build_root.bash VARIANT='$VARIANT' full_build'

    while ssh "$SSH_HOST" '[ -e /build/'$VARIANT'-full_build.working ]'; do {
        echo "waiting"
        sleep 20
    }; done
    scp "$SSH_HOST":$_o/$VARIANT-rootfs.squashfs.xz $_o
    [[ ! "$VARIANT" =~ "host" ]] &&
    scp "$SSH_HOST":$_o/$VARIANT-sdcard.img.xz      $_o
    scp "$SSH_HOST":$_o/$VARIANT-host.tar.xz        $_o
    scp "$SSH_HOST":$_o/$VARIANT-staging.tar.xz     $_o

    unxz -fv $_o/$VARIANT-rootfs.squashfs.xz
    [[ ! "$VARIANT" =~ "host" ]] &&
    unxz -fv $_o/$VARIANT-sdcard.img.xz

    _h=/build/$VARIANT-host/
    rm -rvf    $_h
    mkdir -p   $_h
    tar xJv -C $_h -f $_o/$VARIANT-host.tar.xz

    _h=/build/$VARIANT-staging/
    rm -rvf    $_h
    mkdir -p   $_h
    tar xJv -C $_h -f $_o/$VARIANT-staging.tar.xz

    if [[ "$VARIANT" =~ "host" ]]; then
        _target=x86_64-buildroot-linux-uclibc
    else
        _target=arm-buildroot-linux-uclibcgnueabihf
    fi
    cp -v $_h/lib/gcc/$_target/8.4.0/{crtbeginT.o,crtend.o} \
        $_h/$_target/sysroot/usr/lib/

)

eval "$@"

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

cat << EOF > autoexec.sh
#!/bin/sh

dd if=/dev/hwrng of=/dev/random bs=2048 count=16 &
ip link set dev wlan0 up
ip addr add 192.168.4.31/24 dev wlan0
ip route add default via 192.168.4.1
echo "nameserver 8.8.8.8" > /etc/resolv.conf
wpa_supplicant -B  -i wlan0 -c /etc/wpa_supplicant.conf

# # TODO: wpa_cli -a ....

while true; do {
    echo -e "\n\n\n\n\n"
    ip link
    echo
    ip addr
    sleep 5
}; done
EOF

cat << EOF > etc/wpa_supplicant.conf
update_config=1
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1

network={
    key_mgmt=WPA-PSK
    ssid="test-ap2"
    psk="the-bike-shed"
}
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
console::respawn:/sbin/getty -L  console 0 vt100 # GENERIC_SERIAL
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