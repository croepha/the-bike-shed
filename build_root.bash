echo "Not ready for general consumption, enter at your own risk"
exit

apt install -y cpio rsync sudo ccache

export FORCE_UNSAFE_CONFIGURE=1

mkdir -p /build/root
cd /build/root

BR_VERSION=2020.02.2
wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-$BR_VERSION.tar.gz
tar xvf buildroot.tar.gz --strip-components=1


make clean
make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root.config
make all

make menuconfig
make linux-menuconfig
make busybox-menuconfig
make uclibc-menuconfig

(
    set -eEuo pipefail
    make savedefconfig
    make linux-update-defconfig
    make busybox-update-config
    make uclibc-update-config
)




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
# now run any rc scripts
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


