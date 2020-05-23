set -eEuo pipefail
source /etc/profile

# cp -v /build/pi0w-host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/{crtbeginT.o,crtend.o} \
#   /build/pi0w-host/arm-buildroot-linux-uclibcgnueabihf/sysroot/usr/lib/

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

EOF

function compile() {
cat << EOF >> /build/build.ninja
build /build/$1.c.o: cc $1.c
  extra = -O0 -gfull -fPIC -fsanitize=address ${@:2}
EOF
OBJ_FILES="$OBJ_FILES /build/$1.c.o"
}

function link_exec() {
cat << EOF >> /build/build.ninja
build /build/$1.exec: link_exec $OBJ_FILES
  extra = -fuse-ld=lld -fsanitize=address  ${@:2}
EOF
}
# -target arm-unknown-linux-gnueabihf
# -static

OBJ_FILES=""
compile    helloworld -D SOME_DEFINE=234234
link_exec  helloworld

OBJ_FILES=""
compile    mount_squash_root
link_exec  mount_squash_root

#  --sysroot=/build/root/pi0_usr_include/output/staging/

# PI0W Debug version
# cat << EOF >> /build/build.ninja
# build /build/mount_squash_root.pi0devstatic.c.o: cc mount_squash_root.c
#   extra = --sysroot=/build/pi0w-host/arm-buildroot-linux-uclibcgnueabihf/sysroot -target arm-linux-gnueabihf -O0 -gfull

# build /build/mount_squash_root.pi0devstatic.exec: link_exec /build/mount_squash_root.pi0devstatic.c.o
#   extra = --sysroot=/build/pi0w-host/arm-buildroot-linux-uclibcgnueabihf/sysroot -L /build/pi0w-host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/ -target arm-linux-gnueabihf -fuse-ld=lld -static
# EOF

# output/host/lib/gcc/arm-buildroot-linux-uclibcgnueabihf/8.4.0/

OBJ_FILES=""
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

#/build/helloworld.exec
# bash mount_squash_root_test.bash

bash shed_test_udp.bash