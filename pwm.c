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


#if GPIO_FAKE != 1
static void write_file(char* path, char * content) { int r;
    int fd = open(path, O_WRONLY);
    error_check(fd);
    r = dprintf(fd, "%s", content);
    error_check(r);
    r = close(fd);
    error_check(r);
}
#endif


void gpio_pwm_initialize(void) {
#if GPIO_FAKE == 1
    INFO("GPIO_FAKE");
    pwm0_enable_fd = -2;
#else
    write_file("/sys/class/pwm/pwmchip0/export", "0\n");
    write_file("/sys/class/pwm/pwmchip0/pwm0/period", "20000000\n");
    write_file("/sys/class/pwm/pwmchip0/pwm0/duty_cycle", "18000000\n");
    pwm0_enable_fd = open("/sys/class/pwm/pwmchip0/pwm0/enable", O_WRONLY);
    error_check(pwm0_enable_fd);
#endif
}


void gpio_pwm_set(u8 value) {
    assert(pwm0_enable_fd != -1);
#if GPIO_FAKE == 1
    INFO("GPIO_FAKE value:%d", value);
#else
    DEBUG("%d", value);
    assert(value == 0 || value == 1);
    int r = dprintf(pwm0_enable_fd, "%d\n", value);
    error_check(r);
#endif
}
