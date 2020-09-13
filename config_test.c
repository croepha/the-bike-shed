
#include <string.h>
#include "logging.h"

char * email_from;
char * email_host;
char * email_user_pass;
char * supr_email_rcpt;
char ** supr_child_argv;


usz const config_memory_SIZE = 1<<11;
char config_memory[config_memory_SIZE];
char * const config_memory_end = config_memory + config_memory_SIZE;
char * config_memory_next = config_memory;

void * config_push(void * mem, usz len) {
    char* ret = config_memory_next;
    if (config_memory_next + len < config_memory_end) {
        memcpy(ret, mem, len);
        config_memory_next += len;
    } else {
        ret = 0;
        ERROR("Config out of memory");
    }
    return ret;
}

char* config_push_string(char * str) {
    usz len = strlen(str) + 1;
    return config_push(str, len);
}


int main () {

    email_from         = config_push_string("tmp-from@testtest.test");
    email_host         = config_push_string("smtp://127.0.0.1:8025");
    email_user_pass    = config_push_string("user:pass");
    supr_email_rcpt    = config_push_string("logging@tmp-test.test");
    char* tmp_array[20];
    char** t = tmp_array;
    *t++ = config_push_string("/bin/sh");
    *t++ = config_push_string("-c");
    *t++ = config_push_string("/usr/bin/ping 127.0.0.1 | ts");
    *t++ = 0;
    supr_child_argv = config_push(tmp_array, (u8*)t - (u8*)tmp_array);

    INFO("email_from: '%s'", email_from);
    INFO("email_host: '%s'", email_host);
    INFO("email_user_pass: '%s'", email_user_pass);
    INFO("supr_email_rcpt: '%s'", supr_email_rcpt);
    INFO("supr_child_argv:");
    for (char**c = supr_child_argv; *c; c++) {
        INFO("\t'%s'", *c);
    }


}