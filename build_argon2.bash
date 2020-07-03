set -eEuo pipefail


# git clone --depth=1 https://github.com/P-H-C/phc-winner-argon2.git -b 20190702

cd /tmp/phc-winner-argon2

mkdir -p  /tmp/phc-winner-argon2/src2

cp \
  /tmp/phc-winner-argon2/LICENSE \
  /tmp/phc-winner-argon2/include/argon2.h \
  /tmp/phc-winner-argon2/src/opt.c\
  /tmp/phc-winner-argon2/src/encoding.c\
  /tmp/phc-winner-argon2/src/thread.c\
  /tmp/phc-winner-argon2/src/blake2/blake2b.c\
  /tmp/phc-winner-argon2/src/core.c\
  /tmp/phc-winner-argon2/src/argon2.c\
  /tmp/phc-winner-argon2/src/core.h\
  /tmp/phc-winner-argon2/src/blake2/blake2.h \
  /tmp/phc-winner-argon2/src/blake2/blamka-round-opt.h \
  /tmp/phc-winner-argon2/src/blake2/blake2-impl.h \
  /tmp/phc-winner-argon2/src/encoding.h \
  /tmp/phc-winner-argon2/src/thread.h \
  /workspaces/the-bike-shed/hello_argon2.c \
  /tmp/phc-winner-argon2/src2/



cd /tmp/phc-winner-argon2/src2

OBJ=""
function compile() {
  clang -pthread -I. $1.c -c -o /build/argon2/$1.c.o
  OBJ="$OBJ /build/argon2/$1.c.o"
}

rm -f blake2
ln -s . blake2

mkdir -p /build/argon2
compile opt
compile encoding
compile thread
compile blake2b
compile core
compile argon2
compile hello_argon2
clang -pthread $OBJ -o hello_argon2.exec

#ar rcs libargon2.a src/argon2.o src/core.o src/blake2/blake2b.o src/thread.o src/encoding.o src/opt.o
#sed '/^##.*$/d; s#@PREFIX@#/usr#g; s#@EXTRA_LIBS@#-lrt -ldl#g; s#@UPSTREAM_VER@#ZERO#g; s#@HOST_MULTIARCH@#lib/x86_64-linux-gnu#g; s#@INCLUDE@#include#g;' < 'libargon2.pc.in' > 'libargon2.pc'

# cc -std=c89 -O3 -Wall -g -Iinclude -Isrc -pthread -march=native   -c -o src/opt.o src/opt.c
# cc -std=c89 -O3 -Wall -g -Iinclude -Isrc -pthread -march=native   -c -o src/encoding.o src/encoding.c
# cc -std=c89 -O3 -Wall -g -Iinclude -Isrc -pthread -march=native   -c -o src/thread.o src/thread.c
# cc -std=c89 -O3 -Wall -g -Iinclude -Isrc -pthread -march=native   -c -o src/blake2/blake2b.o src/blake2/blake2b.c
# cc -std=c89 -O3 -Wall -g -Iinclude -Isrc -pthread -march=native   -c -o src/core.o src/core.c
# cc -std=c89 -O3 -Wall -g -Iinclude -Isrc -pthread -march=native   -c -o src/argon2.o src/argon2.c

