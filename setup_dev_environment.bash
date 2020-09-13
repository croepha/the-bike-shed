set -eEuo pipefail

/*

mkdir -p /tmp/re2c
wget  https://github.com/skvadrik/re2c/releases/download/2.0.3/re2c-2.0.3.tar.xz -O /tmp/re2c/re2c.tar.xz
tar -xvf /tmp/re2c/re2c.tar.xz -C /tmp/re2c --strip=1
cd /tmp/re2c
./configure --disable-golang && make -j $(nproc) && make install
*/


source /etc/profile
if [ ! -v ENV_VER ]; then {
        ENV_VER=0
}; fi

if [ $ENV_VER != 3 ]; then {
  cat << EOF >> /etc/profile
export DEBIAN_FRONTEND=noninteractive
export LC_ALL=C
export ENV_VER=3
EOF
  source /etc/profile
}; fi

# rm -f /etc/apt/sources.list.d/shed.list  # for debugging
apt update
apt install -y gnupg ca-certificates
apt-key add - << EOF
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: GnuPG v1.4.12 (GNU/Linux)

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
bXb5c6gCHESH5PXwPU4jQEE7Ib9J6sbk7ZT2Mw==
=j+4q
-----END PGP PUBLIC KEY BLOCK-----
EOF

echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-10 main" > /etc/apt/sources.list.d/shed.list
apt update -o Dir::Etc::sourcelist=/etc/apt/sources.list.d/shed.list -o APT::Get::List-Cleanup="0"
apt upgrade -y
apt install -y build-essential wget unar libtinfo5 \
        ninja-build git clangd-10 clang-10 lld-10 lldb-10 clang-tools-10 \
        libncurses5-dev bzr cvs mercurial subversion unzip bc \
        dosfstools nginx libcurl4-openssl-dev libssl-dev re2c

for i in clangd clang lld lldb; do {
  ln -sfv $i-10 /usr/bin/$i
}; done


# This is to try to prevent the annoying "host verifcation failed" message on fresh dockers...
mkdir -p ~/.ssh/; chmod og=,u=rwx ~/.ssh/
cat << EOF >> ~/.ssh/known_hosts
github.com ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAq2A7hRGmdnm9tUDbO9IDSwBK6TbQa+PXYPCPy6rbTrTtw7PHkccKrpp0yVhp5HdEIcKr6pLlVDBfOLX9QUsyCOV0wzfjIJNlGEYsdlLJizHhbn2mUjvSAHQqZETYP81eFzLQNnPHt4EVVUh7VfDESU84KezmD5QlWpXLmvU31/yMf+Se8xhHTvKSCZIFImWwoG6mbUoWf9nzpIoaSjB+weqqUUmpaaasXVal72J+UX2B+2RPW3RcT0eOzQgqlJL3RKrTJvdsjE3JEAvGq3lGHSZXy28G3skua2SmVi/w4yCE6gbODqnTWlg7+wC604ydGXA8VJiS5ap43JXiUFFAaQ==
EOF

cat << EOF > /etc/nginx/sites-enabled/default
server {
      listen 9160;
      root /;
      autoindex on;
      dav_methods PUT DELETE MKCOL COPY MOVE;
}
EOF
#   dav_methods PUT DELETE MKCOL COPY MOVE;

# in.tftpd -lca 0.0.0.0:9161 /

exit

~/the-bike-shed/build/esp_venv/bin/esptool.py \
  --chip esp32 --port "/dev/cu.SLAB_USBtoUART" \
  --baud 115200 --before "default_reset" --after "hard_reset" \
   write_flash -z --flash_mode "dio" --flash_freq "40m" --flash_size detect \
    0x10000 $BUILD/hello-world.bin
     0x1000 $BUILD/bootloader.bin \
     0x8000 $BUILD/partitions_singleapp.bin \


# ESP32 IDF:
if false; then {
  apt install -y git wget flex bison gperf python-is-python3 python3-pip python3-setuptools cmake \
      ninja-build ccache libffi-dev libssl-dev dfu-util

  export GIT_WORK_TREE=/build/esp-idf
  export GIT_DIR=$GIT_WORK_TREE/.git
  mkdir -p $GIT_WORK_TREE
  git init
  git config -f .gitmodules submodule.<name>.shallow true
  git remote add origin https://github.com/espressif/esp-idf.git
  git fetch --depth=1 --tags origin 'v3.3.2'
  git checkout --recurse-submodules FETCH_HEAD
  cd  $GIT_WORK_TREE
  git submodule update --init --recursive --depth=1
  ./install.sh

  git clone --depth=1 --no-tags https://github.com/espressif/esp-idf.git

}; fi


# Extras:
if false; then {
  yes | unminimize
  apt install -y man apt-file errno \
    iproute2 iputils-ping telnet socat
  apt-file update
}; fi

