
/*

TODO: Better document function of file
TODO: Cleanup

This is a bit of a mess, Copied a bunch of code from Landley's toybox, in the middle
of stripping it down.

The goal here is to make a self-contained, statically compilable binary that sets up
the squashfs mount on boot

REFS:
https://github.com/landley/toybox/blob/6a73e13d75d31da2c3f1145d8487725f0073a4b8/toys/other/switch_root.c
https://github.com/landley/toybox/blob/6a73e13d75d31da2c3f1145d8487725f0073a4b8/toys/lsb/mount.c
https://github.com/landley/toybox/blob/b7265da4ccdfe4d256e72dc1b2a0f6b54e087ad2/toys/other/losetup.c

*/
#if 0

kernel boots with a ramfs root
mkdir -p /newroot /fat
mount sdcard fat into /fat
mount -o loop /fat/root.squashfs /newroot
switchroot oldroot:/mnt newroot:/newroot /init

switch_root newroot init [arg...]


#endif
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/loop.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>

#define LOG_DEBUG

#include "logging.h"


int debug_get_errno(void);
int debug_get_errno(void) { return errno; }

int main(int argc, char**argv) { int r;
  assert(argc == 3);
  char* phsyical_dev    = *++argv;
  char* squash_path     = *++argv;

  INFO("ASDFASDF");

  // mount -o rw -t argv[1] argv[2] /phsyical
  // TODO??: MS_LAZYTIME
  r = mount(phsyical_dev, "/physical", "vfat", MS_NOATIME, 0);
  error_check(r);

  // TODO: Why not actually specify the loop device?
  int lcfd = open("/dev/loop-control", O_RDWR | O_CLOEXEC);
  error_check(lcfd);
  assert(lcfd!=-1);

  //  Create /dev/loop
  int loop_num = ioctl(lcfd, LOOP_CTL_GET_FREE, 0);
  error_check(loop_num);

  char full_loop_path[128];
  snprintf(full_loop_path, sizeof full_loop_path, "/dev/loop%d", loop_num);
  DEBUG("loop_path %s", full_loop_path);
  int loop0 = open(full_loop_path, O_RDWR | O_CLOEXEC);
  error_check(loop0);

  char full_squash_path[128];
  snprintf(full_squash_path, sizeof full_squash_path, "/physical/%s", squash_path);
  int squash_fd = open(full_squash_path, O_RDONLY | O_CLOEXEC);
  error_check(squash_fd);
  r = ioctl(loop0, LOOP_SET_FD, squash_fd);
  error_check(r);

  // TODO??: MS_LAZYTIME
  r = mount(full_loop_path, "/newroot", "squashfs", MS_NOATIME | MS_RDONLY, 0);
  error_check(r);


  r = chdir("/newroot");
  error_check(r);

  // TODO: Remove stuff from / ?

  r = mount("none", "/newroot/mnt", "tmpfs", 0, NULL);
  error_check(r);

  r = mkdir("/newroot/mnt/physical", 0777);
  error_check(r);

  r = mount("/physical", "/newroot/mnt/physical", NULL, MS_MOVE, NULL);
  error_check(r);

  r = mount("/dev", "/newroot/dev", NULL, MS_MOVE, NULL);
  error_check(r);

  r = mount(".", "/", NULL, MS_MOVE, NULL);
  error_check(r);

  r = chroot(".");
  error_check(r);

  r = chdir("/");
  error_check(r);

  r = execl("/busybox", "echo", "asdfasdf", 0);
  error_check(r);

  INFO("Asdfasd DONE ? f");

  // char loopback_dev[1024];
  // loopback_setup(squash_path, loopback_dev, sizeof loopback_dev);
  // printf("GOT LOOPBACK_DEV: %s\n", loopback_dev);

    // mount -o loop -t squashfs /fat/root.squashfs newroot
    // switch_root  new_root:/newroot old_root:/mnt
    // exec /init
}

