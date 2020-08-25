REMOTE=https://github.com/P-H-C/phc-winner-argon2.git
REF=20190702
FILE_LIST=(
  LICENSE
  include/argon2.h
  src/encoding.h
  src/thread.h
  src/core.h
  src/ref.c
  src/encoding.c
  src/thread.c
  src/core.c
  src/argon2.c
  src/blake2/blake2b.c
  src/blake2/blake2.h
  src/blake2/blamka-round-ref.h
  src/blake2/blake2-impl.h
)

export GIT_DIR=/tmp/argon2
git init --bare $GIT_DIR
git remote add origin https://github.com/P-H-C/phc-winner-argon2.git || true
git fetch --depth=1 origin $REF

DST=argon2
rm -rf $DST
mkdir  $DST
for file in "${FILE_LIST[@]}"; do {
  git show FETCH_HEAD:$file > $DST/$( basename $file )
}; done

cd $DST
rm -f blake2
ln -s . blake2
