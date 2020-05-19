
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
#include <assert.h>

// TODO: Credit Landley

// initrd needs:
//  - this exec as init (or rdinit?)
//  - /dev/
//  - /root/
//  - /fat/
// Embed initrd into kernel


void verror_msg(char *msg, int err, va_list va)
{
  char *s = ": %s";

  fprintf(stderr, "%s: ", "FIXME");
  if (msg) vfprintf(stderr, msg, va);
  else s+=2;
  if (err>0) fprintf(stderr, s, strerror(err));
  if (msg || err) putc('\n', stderr);
}

// These functions don't collapse together because of the va_stuff.


void xexit() {
    _exit(-1);
}

void error_msg(char *msg, ...)
{
  va_list va;

  va_start(va, msg);
  verror_msg(msg, 0, va);
  va_end(va);
}

void perror_msg(char *msg, ...)
{
  va_list va;

  va_start(va, msg);
  verror_msg(msg, errno, va);
  va_end(va);
}

// Die with an error message.
void error_exit(char *msg, ...)
{
  va_list va;

  va_start(va, msg);
  verror_msg(msg, 0, va);
  va_end(va);

  xexit();
}

// Die with an error message and strerror(errno)
void perror_exit(char *msg, ...)
{
  // Die silently if our pipeline exited.
  if (errno != EPIPE) {
    va_list va;

    va_start(va, msg);
    verror_msg(msg, errno, va);
    va_end(va);
  }

  xexit();
}


// If you want to explicitly disable the printf() behavior (because you're
// printing user-supplied data, or because android's static checker produces
// false positives for 'char *s = x ? "blah1" : "blah2"; printf(s);' and it's
// -Werror there for policy reasons).
void error_msg_raw(char *msg)
{
  error_msg("%s", msg);
}

void perror_msg_raw(char *msg)
{
  perror_msg("%s", msg);
}

void error_exit_raw(char *msg)
{
  error_exit("%s", msg);
}

void perror_exit_raw(char *msg)
{
  perror_exit("%s", msg);
}



#define FLAG(M) (FLAG_ ## M)


int get_errno() {
  return errno;
}

char toybuf[4096];
// Perform requested operation on one device. Returns 1 if handled, 0 if error
int loopback_setup(char *file, char *device_out, ssize_t device_out_size)
{

  char *device = 0;
  struct loop_info64 *loop = (void *)(toybuf+32);
  int lfd = -1, ffd = ffd;
  int racy = !device;

  char* TT_j = 0;
  int TT_jdev = 0;
  int TT_jino = 0;

  if (TT_j) {
    struct stat st;

    stat(TT_j, &st);
    TT_jdev = st.st_dev;
    TT_jino = st.st_ino;
  }

  int FLAG_a = 0;
  int FLAG_d = 0;
  int FLAG_j = 0;
  int FLAG_f = 1;
  int FLAG_c = 0;
  int FLAG_s = 1;
  int TT_o = 0;
  int TT_S = 0;


  // Open file (ffd) and loop device (lfd)
  char is_readonly = 0;
  int TT_openflags = is_readonly ? O_RDONLY : O_RDWR;

  if (file) ffd = open(file, TT_openflags);
  if (!device) {
    int i;
    int cfd = open("/dev/loop-control", O_RDWR);
    printf("FAIL?: %s\n", strerror(errno));
    assert(cfd != -1);


    // We assume /dev is devtmpfs so device creation has no lag. Otherwise
    // just preallocate loop devices and stay within them.

    // mount -o loop depends on found device being at the start of toybuf.
    if (cfd != -1) {
      if (0 <= (i = ioctl(cfd, LOOP_CTL_GET_FREE))) {
        sprintf(device = toybuf, "/dev/loop%d", i);
      }
      close(cfd);
    }
  }

  assert(device);

  if (device) lfd = open(device, TT_openflags);


  // Stat the loop device to see if there's a current association.
  memset(loop, 0, sizeof(struct loop_info64));
  if (-1 == lfd || ioctl(lfd, LOOP_GET_STATUS64, loop)) {
    if (errno == ENXIO && (FLAG(a) || FLAG(j))) goto done;
    // ENXIO expected if we're just trying to print the first unused device.
    if (errno == ENXIO && FLAG(f) && !file) {
      puts(device);
      goto done;
    }
    if (errno != ENXIO || !file) {
      perror_msg_raw(device ? device : "-f");
      goto done;
    }
  }

  // Skip -j filtered devices
  if (TT_j && (loop->lo_device != TT_jdev || loop->lo_inode != TT_jino))
    goto done;

  // Check size of file or delete existing association
  if (FLAG(c) || FLAG(d)) {
    // The constant is LOOP_SET_CAPACITY
    if (ioctl(lfd, FLAG(c) ? 0x4C07 : LOOP_CLR_FD, 0)) {
      perror_msg_raw(device);
      goto done;
    }
  // Associate file with this device?
  } else if (file) {
    char *f_path = file;

    if (!f_path) perror_exit("file"); // already opened, but if deleted since...
    if (ioctl(lfd, LOOP_SET_FD, ffd)) {
      if (racy && errno == EBUSY) return 1;
      perror_exit("%s=%s", device, file);
    }
    strncpy((char *)loop->lo_file_name, f_path, LO_NAME_SIZE);
    loop->lo_offset = TT_o;
    loop->lo_sizelimit = TT_S;
    if (ioctl(lfd, LOOP_SET_STATUS64, loop)) perror_exit("%s=%s", device, file);
    if (FLAG(s)) puts(device);
    snprintf(device_out, device_out_size, "%s", device);
  }
  else {
    printf("%s: [%lld]:%llu (%s)", device, (long long)loop->lo_device,
      (long long)loop->lo_inode, loop->lo_file_name);
    if (loop->lo_offset) printf(", offset %llu",
      (unsigned long long)loop->lo_offset);
    if (loop->lo_sizelimit) printf(", sizelimit %llu",
      (unsigned long long)loop->lo_sizelimit);
    printf("\n");
  }

done:
  if (file) close(ffd);
  if (lfd != -1) close(lfd);
  return 0;
}

int main() {

  printf("ASDFASDF\n");

  char loopback_dev[1024];
  loopback_setup("/root-dev.squashfs", loopback_dev, sizeof loopback_dev);
  printf("GOT LOOPBACK_DEV: %s\n", loopback_dev);

    // mount -o rw -t vfat /dev/mmcblk0p1 fat
    // mount -o loop -t squashfs /fat/root.squashfs newroot
    // switch_root  new_root:/newroot old_root:/mnt
    // exec /init
}