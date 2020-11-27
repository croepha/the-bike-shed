set -eEuo pipefail
source /etc/profile
# function _default() {_varname=$1; _default_value=$2;
#     eval '
#     if [ ! -v '$_varname' ]; then {
#         '$_varname'="'$_default_value'"
#     }; fi
#     '
# }

mkdir -p /build/


# Start up services used for testing
# pkill email_mock
./email_test_server.py
# nginx -s stop
pgrep nginx > /dev/null || nginx

if [ ! -v SHOULD_CLEAN ]; then {
        SHOULD_CLEAN=0
}; fi

# Setup the pseudo serial interface for mock testing the exterior interface
if ! pgrep -f exterior_mock.pts > /dev/null; then {
tmux new -d 'nohup socat -d -d -v PTY,link=/build/exterior_mock.pts2,raw,echo=0 PTY,link=/build/exterior_mock.pts,raw,echo=0 0<&- >> /build/exterior_mock.log 2>&1 '
}; fi


echo "/tmp/core.%e.%t.%p" > /proc/sys/kernel/core_pattern
# sudo cat ~/Library/Containers/com.docker.docker/Data/vms/0/tty
# screen $( sudo cat ~/Library/Containers/com.docker.docker/Data/vms/0/tty )

rm -f /build/build.ninja
cat << 'EOF' >> /build/build.ninja
builddir = /build/

rule cc
 command = clang -Wmissing-prototypes -Wno-writable-strings -Werror -Wshadow -Wall ./$in -c -o $out -MF $out.d -MMD $extra
 depfile = ${out}.d
 deps = gcc
 description = CC $out

rule link_exec
 command = clang $in -o $out $extra
 description = LINK $out

rule cc_br
 command = ${pi0w_host_prefix}-gcc -Werror -Wshadow-Wall ./$in -c -o $out -MF $out.d -MMD $extra
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
pi0w_host_dir="/build/rootpi0w-dev/host/"
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
  _build dbg      -gfull -O0    -D ABORT_ON_ERROR=1 -D GPIO_FAKE=1 -D BUILD_IS_RELEASE=0 -fPIC -fsanitize=address
  _build fast     -gfull -Ofast -D ABORT_ON_ERROR=0 -D GPIO_FAKE=1 -D BUILD_IS_RELEASE=0 -fPIC -flto=thin -march=native
  _build pi0wdbg  -gfull -O0    -D ABORT_ON_ERROR=1 -D GPIO_FAKE=0 -D BUILD_IS_RELEASE=0 $pi0w_common
  _build pi0wfast -gfull -Ofast -D ABORT_ON_ERROR=0 -D GPIO_FAKE=0 -D BUILD_IS_RELEASE=1 $pi0w_common
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
build /build/$NAME.test_results: test /workspaces/the-bike-shed/test_wrapper.bash $EXEC $NAME.expected_output
EOF
}

function do_test2() {
  NAME=$1
  EXEC=$2
  OTHER_DEPS=$3
cat << EOF >> /build/build.ninja
build /build/$NAME.test_results: test /workspaces/the-bike-shed/test_wrapper.bash $EXEC $NAME.expected_output | $OTHER_DEPS
EOF
}

do_test2 mount_squash_root_test /workspaces/the-bike-shed/mount_squash_root_test.bash /build/mount_squash_root.dbg.exec


# cat << EOF >> /build/build.ninja
# build /build/email_test.test_results: test /workspaces/the-bike-shed/email_test.bash /build/email_test.dbg.exec email_test.expected_output
# EOF


# -target arm-unknown-linux-gnueabihf
# -static

reset
depends_on logging
compile    mount_squash_root
link_exec  mount_squash_root

reset
FLAVOR=static
compile    logging    -fno-sanitize=address
compile    mount_squash_root -fno-sanitize=address
link_exec  mount_squash_root -fno-sanitize=address -static


# compile    logging -D 'LOGGING_USE_EMAIL=1'
reset
depends_on logging
compile    io_core
compile    io_curl
compile    io_curl_test
depends_on config_download
link_exec  io_curl_test -l curl -l crypto


reset
compile    misc
compile    misc_test
depends_on logging
link_exec   misc_test
do_test misc_test /build/misc_test.dbg.exec

reset
compile argon2/ref       -Iargon2
compile argon2/encoding  -Iargon2
compile argon2/thread    -Iargon2
compile argon2/blake2b   -Iargon2
compile argon2/core      -Iargon2
compile argon2/argon2    -Iargon2
compile access_hash      -Iargon2
compile access_hash_test
depends_on logging
link_exec access_hash_test    -l pthread
do_test access_hash_test /build/access_hash_test.fast.exec

# do_test test_shed /build/shed3.dbg.exec

reset
compile logging

reset
depends_on logging
depends_on supervisor_email
compile supervisor_email_test
link_exec supervisor_email_test -l curl
do_test supervisor_email_test /build/supervisor_email_test.dbg.exec

reset
depends_on  logging
compile     supervisor_io
compile     supervisor_io_test
depends_on  io_core
depends_on  misc
link_exec   supervisor_io_test
do_test supervisor_io_test /build/supervisor_io_test.dbg.exec

reset
depends_on  logging
depends_on  io_core
depends_on  io_curl
depends_on  supervisor_io
FLAVOR=nodiagnostic depends_on  config
depends_on  misc
compile     email
compile     supervisor_email
compile     supervisor
link_exec   supervisor -l curl


reset
depends_on logging
FLAVOR=diagnostic compile    config -D CONFIG_DIAGNOSTICS=1
compile    config_diag_test
link_exec  config_diag_test
do_test    config_diag_test /build/config_diag_test.dbg.exec

reset
depends_on logging
FLAVOR=nodiagnostic compile    config -D CONFIG_DIAGNOSTICS=0
compile    config_nodiag_test
link_exec  config_nodiag_test
do_test    config_nodiag_test /build/config_nodiag_test.dbg.exec

reset
depends_on logging
depends_on io_core
depends_on misc
compile    io_core_test
link_exec  io_core_test

reset
depends_on logging
depends_on io_core
depends_on io_curl
depends_on email
depends_on misc
compile    io_test_full
link_exec  io_test_full -l curl


reset
depends_on logging
depends_on email
compile   email_test
link_exec email_test -l curl

do_test email_test   /build/email_test.dbg.exec
do_test io_core_test /build/io_core_test.dbg.exec
do_test io_curl_test /build/io_curl_test.dbg.exec
do_test io_test_full /build/io_test_full.dbg.exec


reset
depends_on logging
compile line_accumulator
compile line_accumulator_test
link_exec line_accumulator_test
do_test line_accumulator_test /build/line_accumulator_test.dbg.exec

reset
depends_on logging
depends_on line_accumulator
depends_on io_core
depends_on io_curl
compile config_download
compile config_download_test2
link_exec config_download_test2 -l curl
do_test config_download_test2 /build/config_download_test2.dbg.exec

reset
depends_on logging
FLAVOR=diagnostic depends_on    config
compile    config_validator
link_exec  config_validator

reset
depends_on logging
compile access_data
compile access_data_test
link_exec access_data_test
do_test access_data_test /build/access_data_test.dbg.exec


re parse_headers
re config
re exterior_protocol

#  --sysroot=/build/root/pi0_usr_include/output/staging//build/email_test.dbg.exec

# cat << EOF >> /build/build.ninja
# build /build/mount_squash_root.pi0devstatic.c.o: cc mount_squash_root.c
#   extra = $pi0w_common -O0 -gfull

# build /build/mount_squash_root.pi0devstatic.exec: link_exec /build/mount_squash_root.pi0devstatic.c.o
#   extra = $pi0w_common -L $pi0w_host_lib -fuse-ld=lld -static
# EOF

reset
depends_on logging
compile serial_test
depends_on line_accumulator
compile serial_open
link_exec serial_test -l util
do_test serial_test /build/serial_test.dbg.exec


reset
depends_on logging
depends_on io_core
depends_on serial_open
depends_on misc
depends_on line_accumulator
compile serial_hw_test
compile serial_io
link_exec serial_hw_test

reset
depends_on logging
depends_on io_core
depends_on serial_open
depends_on misc
depends_on line_accumulator
depends_on email
depends_on io_curl
depends_on serial_io
depends_on argon2/ref
depends_on argon2/encoding
depends_on argon2/thread
depends_on argon2/blake2b
depends_on argon2/core
depends_on argon2/argon2
depends_on access_hash
depends_on access_data
depends_on config_download
FLAVOR=nodiagnostic depends_on config
compile pwm
compile shed3
link_exec shed3 -l curl -l pthread


# The toolchain generated by Buildroot is located by default in
# +output/host/+. The simplest way to use it is to add
# +output/host/bin/+ to your PATH environment variable and then to
# use +ARCH-linux-gcc+, +ARCH-linux-objdump+, +ARCH-linux-ld+, etc.

# pi0w_common="--sysroot=/build/pi0w-dev-staging/ $pi0w_target_flags"



# output/host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/


export NINJA_STATUS="[%f/%t %e] "
ninja -f /build/build.ninja -t compdb > compile_commands.json
if ! diff compile_commands.json /build/old_compile_commands.json &> /dev/null; then {
    pkill clangd || true # Force a restart of clangd when compile_commands.json changes
    cp compile_commands.json /build/old_compile_commands.json
}; fi

if [ $SHOULD_CLEAN != 0 ]; then {
    ninja -f /build/build.ninja -t clean
}; fi


ninja -f /build/build.ninja -k 0 | cat
# clang-check-10 -fixit --fix-what-you-can *.c


#/build/helloworld.exec
# bash mount_squash_root_test.bash
# bash shed_test_udp.bash

