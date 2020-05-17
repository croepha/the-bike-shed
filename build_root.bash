echo "Not ready for general consumption, enter at your own risk"
exit

# Notes
# For a new buildroot project:
#  dl, extract
#  make defconfig <base>
#  make menuconfig ... update all the def config options
#  then do all the saveconfigs...


apt install -y cpio rsync sudo ccache
useradd notroot

mkdir -p /build/root
chown -R notroot /build
cd /build/root

wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2020.05-rc1.tar.gz
tar xvf buildroot.tar.gz
mv /build/root/buildroot-2020.05-rc1/{.,}* /build/root/
rmdir /build/root/buildroot-2020.05-rc1

make clean
make defconfig BR2_DEFCONFIG=/workspaces/the-bike-shed/root.config
make

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



make O=/build/root1/ raspberrypi_defconfig
make O=/build/root1/ savedefconfig
