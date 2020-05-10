echo "Not ready for general consumption, enter at your own risk"
exit

mkdir -p /build/root
cd /build/root

wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2020.05-rc1.tar.gz
tar xvf buildroot.tar.gz

cd /build/root/buildroot-2020.05-rc1/

ln -sf /workspaces/the-bike-shed/buildroot.config .config
