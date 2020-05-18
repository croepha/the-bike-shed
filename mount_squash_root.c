

#if 0

kernel boots with a ramfs root
mkdir -p /newroot /fat
mount sdcard fat into /fat
mount -o loop /fat/root.squashfs /newroot
switchroot oldroot:/mnt newroot:/newroot /init

switch_root newroot init [arg...]


#endif


// TODO: Credit Landley



int main() {


}