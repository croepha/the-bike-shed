
export FORCE_UNSAFE_CONFIGURE=1
export GIT_WORK_TREE=/workspaces/the-bike-shed/
export GIT_DIR=$GIT_WORK_TREE/.git
export XZ_OPT="-9e --threads=0 -v"
#export _F="set -eEuo pipefail"
export BR_VERSION=2020.02.2

function _F() {
    set -eEuo pipefail
    _b=/build/$VARIANT
    _w=/workspaces/the-bike-shed/
    _i=$_b/images/
    _o=$_w/build/
}

function make() ( _F
    /usr/bin/make -C /build/root O=$_b "$@"
)


function build_toolchain() (
    VARIANT=toolchain_pi0w
    _F
#    make defconfig BR2_DEFCONFIG=$_w/pi0w-dev-root.config
    make sdk
)

function build_pi0w2() (
    VARIANT=root_pi0w2
    _F
    mkdir -p $_b
    #make defconfig BR2_DEFCONFIG=$_w/pi0w-dev-root.config
    # make menuconfig
    make
)


set -x
eval "$@"
