echo "Not ready for general consumption, enter at your own risk"
exit

mkdir -p /buildroot
cd build/root

wget -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2020.05-rc1.tar.gz
tar xvf buildroot.tar.gz
