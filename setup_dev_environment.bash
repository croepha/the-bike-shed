set -eEuo pipefail

LLVM_URL=https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz

source /etc/profile
if [ ! -v ENV_VER ]; then {
        ENV_VER=0
}; fi

if [ $ENV_VER != 2 ]; then {
  cat << EOF >> /etc/profile
export DEBIAN_FRONTEND=noninteractive
export LC_ALL=C
export PATH=/opt/llvm/bin:\$PATH
export ENV_VER=2
EOF
  source /etc/profile
}; fi

apt update
apt upgrade -y
apt install -y build-essential wget unar libtinfo5 \
        ninja-build git

if [ ! -e /opt/llvm ]; then {
    wget $LLVM_URL -O /tmp/llvm.tar.xz
    mkdir -p /opt/llvm
    tar xvf /tmp/llvm.tar.xz --strip 1 -C /opt/llvm
    rm /tmp/llvm.tar.xz
}; fi

