echo "Not ready for general consumption, enter at your own risk"
exit

apt install -y cpio rsync sudo ccache
useradd notroot

mkdir -p /build/root
chown -R notroot /build /workspaces
sudo -u notroot bash
cd /build/root

wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2020.05-rc1.tar.gz
tar xvf buildroot.tar.gz
mv /build/root/buildroot-2020.05-rc1/{.,}* /build/root/
rmdir /build/root/buildroot-2020.05-rc1

make clean
make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root.config
make all

BR2_PACKAGE_BUSYBOX_CONFIG

sudo -u notroot bash
cd /build/root
make menuconfig
make savedefconfig

BR2_LINUX_KERNEL_DEFCONFIG

make linux-menuconfig
make linux-update-defconfig

make busybox-menuconfig
make busybox-update-config

make uclibc-menuconfig
make uclibc-update-config


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
 - Maybe switch to using IWD?
 - busybox dns?
 - allow dhcp
 - Add nano (from busybox???)
 - Set static IP
 - TODO output IP Address
 - Disable serial port in inittab
 - Remove logging daemon?
"

while true; do {
    echo -e "\n\n\n\n\n"
    ip a
    sleep 5
}; done



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

ln -sf /mnt/physical/network_interfaces  etc/network/interfaces

cat << EOF > physical/network_interfaces
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet static
    address 192.168.4.30/24
    gateway 192.168.4.1
EOF
#     post-up echo "nameserver 8.8.8.8" > /etc/resolv.conf


cat << EOF > root/etc/inittab

# /etc/inittab
#
# Copyright (C) 2001 Erik Andersen <andersen@codepoet.org>
#
# Note: BusyBox init doesn't support runlevels.  The runlevels field is
# completely ignored by BusyBox init. If you want runlevels, use
# sysvinit.
#
# Format for each entry: <id>:<runlevels>:<action>:<process>
#
# id        == tty to run on, or empty for /dev/console
# runlevels == ignored
# action    == one of sysinit, respawn, askfirst, wait, and once
# process   == program to run

# Startup the system
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
tty1::respawn:/sbin/getty -L  tty1 0 vt100 # HDMI console

# Stuff to do for the 3-finger salute
#::ctrlaltdel:/sbin/reboot

# Stuff to do before rebooting
::shutdown:/etc/init.d/rcK
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r



