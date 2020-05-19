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


