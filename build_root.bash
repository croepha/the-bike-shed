echo "Not ready for general consumption, enter at your own risk"
exit

mkdir -p /build/root
cd /build/root

wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2020.05-rc1.tar.gz


tar xvf buildroot.tar.gz


mkdir -p /build/root/config
make O=/build/root/build -C /build/root/buildroot-2020.05-rc1/


cd /build/root/buildroot-2020.05-rc1/

ln -sf /workspaces/the-bike-shed/buildroot.config .config


apt install cpio rsync sudo

useradd notroot
rm -rvf /build/root/build
mkdir -p /build/root/build
chown -R notroot /build/root/buildroot-2020.05-rc1/
cd /build/root/buildroot-2020.05-rc1/
sudo -u notroot bash

make menuconfig