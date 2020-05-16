echo "Not ready for general consumption, enter at your own risk"
exit

apt install cpio rsync sudo
useradd notroot

mkdir -p /build/root
chown -R notroot /build/root/
cd /build/root
sudo -u notroot bash

wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2020.05-rc1.tar.gz
cd /build/root/buildroot-2020.05-rc1
make raspberrypi0_defconfig
make


make menuconfig


make savedefconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root.config
