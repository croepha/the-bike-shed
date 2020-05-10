set -eEuo pipefail
source /etc/profile

if [ ! -v SHOULD_CLEAN ]; then {
        SHOULD_CLEAN=0
}; fi

mkdir -p /build/

rm -f /build/build.ninja
cat << 'EOF' >> /build/build.ninja
builddir = /build/

rule cc
 command = clang -O0 -gfull -fPIC -fsanitize=address -Wno-writable-strings -Werror -Wshadow -Wall $in -c -o $out -MF $out.d -MMD $extra
 depfile = ${out}.d
 deps = gcc
 description = CC $out

rule link_exec
 command = clang -fuse-ld=lld $in -fsanitize=address -o $out $extra
 description = LINK $out

EOF

function compile() {
cat << EOF >> /build/build.ninja
build /build/$1.c.o: cc $1.c
  extra = ${@:2}
EOF
OBJ_FILES="$OBJ_FILES /build/$1.c.o"
}

function link_exec() {
cat << EOF >> /build/build.ninja
build /build/$1.exec: link_exec $OBJ_FILES
  extra = ${@:2}
EOF
}

OBJ_FILES=""
compile    helloworld -D SOME_DEFINE=234234
link_exec  helloworld

OBJ_FILES=""
compile    shed
link_exec  shed

export NINJA_STATUS="[%f/%t %e] "
ninja -f /build/build.ninja -t compdb > compile_commands.json
if ! diff compile_commands.json /build/old_compile_commands.json &> /dev/null; then {
    pkill clangd # Force a restart of clangd when compile_commands.json changes
    cp compile_commands.json /build/old_compile_commands.json
}; fi

if [ $SHOULD_CLEAN != 0 ]; then {
    ninja -f /build/build.ninja -t clean
}; fi


ninja -f /build/build.ninja | cat

/build/helloworld.exec
