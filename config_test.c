
#include <string.h>
#include <ctype.h>
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



#include "/build/config.re.c"

    // for (char * c=buf; *c; c++) { *c=tolower(*c); }

char*   valid_email_address0 = "EmailAddress: tmp-from@testtest.test";
char*   valid_email_server[] = {
    "EmailServer:  smtps://smtp.gmail.com",
    "EmailServer:  smtps://smtp.gmail.com:465",
    "EmailServer:  smtp://127.0.0.1",
    "EmailServer:  smtp://127.0.0.1:8025",
    "EmailServer:  smtps://127.0.0.1:8025",
};
char* invalid_email_server[] = {
    "EmailServer:  smtp.gmail.com",
    "EmailServer:  smtp://127.0.0.1:",
    "EmailServer:  smtpss://127.0.0.1:",
    "EmailServer:  smtpss://:333",
    "EmailServer:  smtps://127.0.0.1:8ddd025",
};

void _test_set(char**set, usz set_len) {
    char buf[1024];
    for (int i=0; i < set_len; i++) {
        email_host = 0;
        INFO("Trying line: '%s'", set[i]);
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        parse_config(buf);
        INFO("Effective: '%s' Failures: %d", email_host, 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
}

#define test_set(set) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set( set, COUNT(set)); }
int main () {

    test_set(  valid_email_server);
    test_set(invalid_email_server);

    char buf[1024];

    for (int i=0; i < COUNT(valid_email_server); i++) {
        email_host = 0;
        INFO("Trying line: '%s'", valid_email_server[i]);
        strcpy(buf, valid_email_server[i]);
        parse_config(buf);
        log_allowed_fails = 100;
        parse_config(buf);
        INFO("Effective: '%s' Failures: %d", email_host, 100 - log_allowed_fails);
    }


    strcpy(buf, valid_email_address0);  parse_config(buf);

    char buf3[] = "EmailServer: smasdfadtp://127.0.0.1:8025";
    parse_config(buf3);

    //email_host         = config_push_string("smtp://127.0.0.1:8025");
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