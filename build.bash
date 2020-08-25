set -eEuo pipefail
source /etc/profile

# function _default() {_varname=$1; _default_value=$2;
#     eval '
#     if [ ! -v '$_varname' ]; then {
#         '$_varname'="'$_default_value'"
#     }; fi
#     '
# }

# Start up services used for testing
# pkill email_mock
./email_test_server.py
# nginx -s stop
pgrep nginx > /dev/null || nginx

if [ ! -v SHOULD_CLEAN ]; then {
        SHOULD_CLEAN=0
}; fi

mkdir -p /build/

rm -f /build/build.ninja
cat << 'EOF' >> /build/build.ninja
builddir = /build/

rule cc
 command = clang -Wno-writable-strings -Werror -Wshadow -Wall $in -c -o $out -MF $out.d -MMD $extra
 depfile = ${out}.d
 deps = gcc
 description = CC $out

rule link_exec
 command = clang $in -o $out $extra
 description = LINK $out

rule cc_br
 command = ${pi0w_host_prefix}-gcc -Werror -Wshadow -Wall $in -c -o $out -MF $out.d -MMD $extra
 depfile = ${out}.d
 deps = gcc
 description = CCBR $out

rule link_br_exec
 command = ${pi0w_host_prefix}-gcc $in -o $out $extra
 description = LINKBR $out


rule re
 command = re2c -W --tags $in -o $out
 description = RE $out


EOF


pi0w_target_flags="-target arm-linux-gnueabihf -mfloat-abi=hard -mcpu=arm1176jzf-s -mfpu=vfpv2"
# PI0W Debug version
pi0w_host_dir="/build/pi0w-dev-host/"
pi0w_host_lib="$pi0w_host_dir/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/"
pi0w_sysroot="$pi0w_host_dir/arm-buildroot-linux-uclibcgnueabihf/sysroot"
pi0w_common="--sysroot=$pi0w_sysroot $pi0w_target_flags"
echo "pi0w_host_prefix=$pi0w_host_dir/bin/arm-buildroot-linux-uclibcgnueabihf" >> /build/build.ninja


_OBJ_ONLY=0

function _build() { VARIANT=$1; _O="/build/$SOURCE.c.${FLAVOR}${VARIANT}.o"
if [ $_OBJ_ONLY = 0 ]; then {
  echo -e "build $_O: cc $SOURCE.c\n    extra = ${@:2} ${ARGS[@]}" >> /build/build.ninja
}; fi
eval "${VARIANT}"'_OBJ_FILES="$'"${VARIANT}"'_OBJ_FILES $_O"'
}

function compile() { SOURCE="$1"; ARGS=("${@:2}")
  _build dbg      -gfull -O0    -D ABORT_ON_ERROR=1 -D BUILD_IS_RELEASE=0 -fPIC -fsanitize=address
  _build fast     -gfull -Ofast -D ABORT_ON_ERROR=0 -D BUILD_IS_RELEASE=0 -fPIC -flto=thin -march=native
  _build pi0wdbg  -gfull -O0    -D ABORT_ON_ERROR=1 -D BUILD_IS_RELEASE=0 $pi0w_common
  _build pi0wfast -gfull -Ofast -D ABORT_ON_ERROR=0 -D BUILD_IS_RELEASE=1 $pi0w_common
}

function depends_on() {
  _OBJ_ONLY=1 compile "$@"
}

function link_exec() {
cat << EOF >> /build/build.ninja
build /build/$1.${FLAVOR}dbg.exec: link_exec $dbg_OBJ_FILES
  extra = -gfull -fuse-ld=lld -fsanitize=address  ${@:2}
build /build/$1.${FLAVOR}fast.exec: link_exec $fast_OBJ_FILES
  extra = -gfull -fuse-ld=lld -flto=thin -march=native ${@:2}
build /build/$1.${FLAVOR}pi0wdbg.exec: link_br_exec $pi0wdbg_OBJ_FILES
  extra = -ggdb3 -O0 ${@:2}
build /build/$1.${FLAVOR}pi0wfast.exec: link_br_exec $pi0wfast_OBJ_FILES
  extra = -ggdb3 -O3 ${@:2}
EOF
}

function re() {
  c="/build/$1.re.c"
cat << EOF >> /build/build.ninja
build $c: re $1.re
# build $c${FLAVOR}.o: cc $c
#   extra = -O0 -gfull -fPIC -fsanitize=address ${@:2}
EOF
}

function reset() {
  dbg_OBJ_FILES=""
  fast_OBJ_FILES=""
  pi0wdbg_OBJ_FILES=""
  pi0wfast_OBJ_FILES=""
  FLAVOR=""
}

cat << 'EOF' >> /build/build.ninja
rule test
 command = $in $out
 description = TEST $out
EOF

function do_test() {
  NAME=$1
  EXEC=$2
cat << EOF >> /build/build.ninja
build /build/$NAME.test_results: test /workspaces/the-bike-shed/io_core_test.bash $EXEC $NAME.expected_output
EOF
}

cat << EOF >> /build/build.ninja
build /build/email_test.test_results: test /workspaces/the-bike-shed/email_test.bash /build/email_test.dbg.exec email_test.expected_output
EOF


# -target arm-unknown-linux-gnueabihf
# -static

reset
compile    helloworld -D SOME_DEFINE=234234
link_exec  helloworld

reset
compile    mount_squash_root
link_exec  mount_squash_root

reset
FLAVOR=static
compile    mount_squash_root -fno-sanitize=address
link_exec  mount_squash_root -fno-sanitize=address -static


# compile    logging -D 'LOGGING_USE_EMAIL=1'
reset
FLAVOR=test depends_on logging
depends_on email
compile    misc
compile    io_core
compile    io_curl
compile    io_curl_test
link_exec  io_curl_test -l curl -l crypto

reset
compile argon2/ref       -Iargon2
compile argon2/encoding  -Iargon2
compile argon2/thread    -Iargon2
compile argon2/blake2b   -Iargon2
compile argon2/core      -Iargon2
compile argon2/argon2    -Iargon2
compile hello_argon2     -Iargon2
link_exec hello_argon2 -l pthread

reset
FLAVOR=test compile logging -D 'LOGGING_USE_EMAIL=0'

reset
FLAVOR=test depends_on logging
depends_on io_core
depends_on misc
compile    io_core_test
link_exec  io_core_test

reset
FLAVOR=test depends_on logging
compile   email
compile   email_test
link_exec email_test -l curl


do_test io_core_test /build/io_core_test.dbg.exec
do_test io_curl_test /build/io_curl_test.dbg.exec
do_test config_download_test2 /build/config_download_test2.dbg.exec

reset
FLAVOR=test depends_on logging
compile config_download_test2
link_exec config_download_test2

reset
FLAVOR=test depends_on logging
compile local_config
link_exec local_config_test


re parse_headers

#  --sysroot=/build/root/pi0_usr_include/output/staging/

# cat << EOF >> /build/build.ninja
# build /build/mount_squash_root.pi0devstatic.c.o: cc mount_squash_root.c
#   extra = $pi0w_common -O0 -gfull

# build /build/mount_squash_root.pi0devstatic.exec: link_exec /build/mount_squash_root.pi0devstatic.c.o
#   extra = $pi0w_common -L $pi0w_host_lib -fuse-ld=lld -static
# EOF

reset
FLAVOR=test depends_on logging
compile serial_test
link_exec serial_test -l util
do_test serial_test /build/serial_test.dbg.exec

# The toolchain generated by Buildroot is located by default in
# +output/host/+. The simplest way to use it is to add
# +output/host/bin/+ to your PATH environment variable and then to
# use +ARCH-linux-gcc+, +ARCH-linux-objdump+, +ARCH-linux-ld+, etc.

# pi0w_common="--sysroot=/build/pi0w-dev-staging/ $pi0w_target_flags"


reset
compile   hello_pwm
link_exec hello_pwm

reset
compile   hello_led
link_exec hello_led

reset
compile   hello_lcd
link_exec hello_lcd



# output/host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/

reset
compile    shed
link_exec  shed

export NINJA_STATUS="[%f/%t %e] "
ninja -f /build/build.ninja -t compdb > compile_commands.json
if ! diff compile_commands.json /build/old_compile_commands.json &> /dev/null; then {
    pkill clangd || true # Force a restart of clangd when compile_commands.json changes
    cp compile_commands.json /build/old_compile_commands.json
}; fi

if [ $SHOULD_CLEAN != 0 ]; then {
    ninja -f /build/build.ninja -t clean
}; fi


ninja -f /build/build.ninja | cat
# clang-check-10 -fixit --fix-what-you-can *.c


#/build/helloworld.exec
# bash mount_squash_root_test.bash
# bash shed_test_udp.bash

