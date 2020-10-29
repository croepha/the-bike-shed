#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "logging.h"
#include "pwm.h"

int pwm0_enable_fd = -1;

static void write_file(char* path, char * content) { int r;
    int fd = open(path, O_WRONLY);
    error_check(fd);
    r = dprintf(fd, "%s", content);
    error_check(r);
    r = close(fd);
    error_check(r);
}

void gpio_pwm_initialize(void) {
    write_file("/sys/class/pwm/pwmchip0/export", "0\n");
    write_file("/sys/class/pwm/pwmchip0/pwm0/period", "20000000\n");
    write_file("/sys/class/pwm/pwmchip0/pwm0/duty_cycle", "18000000\n");
    pwm0_enable_fd = open("/sys/class/pwm/pwmchip0/pwm0/enable", O_WRONLY);
    error_check(pwm0_enable_fd);
}


void gpio_pwm_set(u8 value) {
    INFO("%d", value);
    assert(value == 0 || value == 1);
    int r = dprintf(pwm0_enable_fd, "%d\n", value);
    error_check(r);
}
