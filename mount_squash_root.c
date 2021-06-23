
/*

This is to make a self-contained, statically compilable binary that sets up
the squashfs mount on boot, this allows us to basically just have a single
vfat parition on the sdcard, and we don't have to fool around with partitions
or extracting the root fs on upgrades... with a little more setup we can get
inplace system upgrades by just copying a new squashfs image over

This binary gets embedded into the linux kernel via the initramfs overlay feature

For quick iterations, this is a shortcut to rebuild a new kernel:

bash build_root.bash build_linux_with_init /build/mount_squash_root.staticpi0wdbg.exec

*/

#define _GNU_SOURCE
#include <sys/sysmacros.h>
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
#include <sys/types.h>
#include <dirent.h>

//#define LOG_DEBUG

#include "logging.h"

// For some reason getting the errno with a debugger is a huge pain in the
//  ass... so we just define a function that we can call that returns it
int debug_get_errno(void);
int debug_get_errno(void) { return errno; }

int main(int argc, char**argv) { int r;

  //  root (/) is the initramfs tmpfs, it should contain:
  //    /init       -- this binary
  //    /dev/       -- empty
  //    /physical/  -- empty
  //    /newroot/   -- empty

  // Mount devtmpfs on /dev
  r = mount("none", "/dev", "devtmpfs", 0, 0);
  error_check(r);

  // If we are doing a production build
#ifdef BUILD_FLAVOR_STATIC
  // Setup standard output, since we're the first process we
  //  can't rely on fd0 to be console
  stdout = stderr = fopen("/dev/console", "we");
#endif
  // if we aren't doing a production build we are a test build
  //  so we probably just want to print to exiting stdout/stderr

  // If were printing something, then its probably an exceptional
  //   situation, lets turn off all the buffering so that the output
  //   isn't lost if were crashing..
  setbuf(stderr, 0);
  setbuf(stdout, 0);

  assert(argc == 3);
  char* phsyical_dev    = *++argv;
  char* squash_path     = *++argv;

  // mount -o rw,sync -t vfat <squash_path> /phsyical
  //   We are mounting with sync because we don't want
  //   any writes to be lost on power loss, this is ok
  //   because we are not a write heavy application, we
  //   only do writes at rare critical times
  // TODO??: MS_LAZYTIME


  int progress_report = 0;

  fprintf(stderr, "\n");

  for (;;) {
    if (progress_report == 0 || progress_report > 5000) {
      fprintf(stderr, "\rWaiting for physical: ");
      progress_report = 0;
    }
    if (progress_report % 50 == 0) {
      fprintf(stderr, ".");
    }
    progress_report++;
    r = mount(phsyical_dev, "/physical", "vfat", MS_NOATIME | MS_SYNCHRONOUS | MS_DIRSYNC, 0);
    if (!(r == -1 && errno == ENOENT)) {
      break;
    }
    usleep(1000);
  }
  fprintf(stderr, " Done\n");

  error_check(r);

  int lcfd = open("/dev/loop-control", O_RDWR | O_CLOEXEC);
  error_check(lcfd);

  //  Create /dev/loopXXX
  int loop_num = ioctl(lcfd, LOOP_CTL_GET_FREE, 0);
  error_check(loop_num);

  // Open /dev/loopXXX
  char full_loop_path[128];
  snprintf(full_loop_path, sizeof full_loop_path, "/dev/loop%d", loop_num);
  DEBUG("loop_path %s", full_loop_path);
  int loop0 = open(full_loop_path, O_RDWR | O_CLOEXEC);
  error_check(loop0);

  // Open (<physical_dev>)/<squash_path>
  char full_squash_path[128];
  snprintf(full_squash_path, sizeof full_squash_path, "/physical/%s", squash_path);
  int squash_fd = open(full_squash_path, O_RDONLY | O_CLOEXEC);
  error_check(squash_fd);
  // Mount (<physical_dev>)/<squash_path> to /dev/loopXXX
  r = ioctl(loop0, LOOP_SET_FD, squash_fd);
  error_check(r);

  // TODO??: MS_LAZYTIME
  // Mount /dev/loopXXX (which is the root squashfs) onto /newroot
  r = mount(full_loop_path, "/newroot", "squashfs", MS_NOATIME | MS_RDONLY, 0);
  error_check(r);

  r = unlink("/init"); // cleanup a little bit
  error_check(r);

  // Mount a writable tmpfs on (root squash)/mnt since root squash is read only
  r = mount("none", "/newroot/mnt", "tmpfs", 0, NULL);
  error_check(r);

  // mkdir (root squash)/mnt/physical
  r = mkdir("/newroot/mnt/physical", 0777);
  error_check(r);

  // Move /physical/ mount to (root squash)/mnt/physical/
  r = mount("/physical", "/newroot/mnt/physical", NULL, MS_MOVE, NULL);
  error_check(r);

  // Move /dev/ mount to (root squash)/dev/
  r = mount("/dev", "/newroot/dev", NULL, MS_MOVE, NULL);
  error_check(r);

  // Move /newroot/ mount to /
  r = chdir("/newroot");
  error_check(r);
  r = mount(".", "/", NULL, MS_MOVE, NULL);
  error_check(r);

  // I'm not an expert, but I think these are needed to make /. and /..
  //  point to eachother, and also to make sure that the execution environment
  //  has realized that we are no longer at /newroot
  r = chroot(".");
  error_check(r);
  r = chdir("/");
  error_check(r);

  INFO("Exec init");

  fflush(stdout);
  fflush(stderr);
  r = execl("/sbin/init", "init", 0);
  error_check(r);

}

