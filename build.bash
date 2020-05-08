set -eEuo pipefail
source /etc/profile
mkdir -p build/

rm -f build/build.ninja
cat << 'EOF' >> build/build.ninja
builddir = build/

rule cc
 command = clang -O0 -gfull -fPIC -fsanitize=address -Wno-writable-strings -Werror -Wshadow -Wall $in -c -o $out -MF $out.d -MMD
 depfile = ${out}.d
 deps = gcc
 description = CC $out

rule link_exec
 command = clang -fuse-ld=lld $in -fsanitize=address -o $out $libs
 description = LINK $out

EOF

function compile() {
cat << EOF >> build/build.ninja
build build/$1.c.o: cc $1.c
EOF
OBJ_FILES="$OBJ_FILES build/$1.c.o"
}

function link_exec() {
cat << EOF >> build/build.ninja
build build/$1.exec: link_exec $OBJ_FILES
EOF
}

OBJ_FILES=""
compile    helloworld
link_exec  helloworld

export NINJA_STATUS="[%f/%t %e] "
ninja -f ./build/build.ninja -t compdb > compile_commands.json
# ninja -f ./build/build.ninja -t clean
ninja -f ./build/build.ninja | cat

./build/helloworld.exec
