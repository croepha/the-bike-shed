echo "Not ready for general consumption, enter at your own risk"
exit

apt install cpio rsync sudo ccache
useradd notroot

mkdir -p /build/root
chown -R notroot /build
cd /build/root
sudo -u notroot bash

wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2020.05-rc1.tar.gz
cd /build/root/buildroot-2020.05-rc1
make clean
make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root.config
make



sudo -u notroot bash
cd /build/root/buildroot-2020.05-rc1
make menuconfig
make savedefconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root.config

