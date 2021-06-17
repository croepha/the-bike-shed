//#define LOG_DEBUG
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"
#include "logging.h"
#include "config.h"
#include "access.h"

void string_list_initialize(struct StringList * sl) {
    sl->first = 0; sl->nextp = &sl->first; sl->count = 0;
}

void string_list_append(struct StringList * sl, char * str, struct StringListLink * sll) {
        *sl->nextp = sll;
        sl->nextp = &(*sl->nextp)->next;
        sl->count++;
        sll->next = 0;
        sll->str = str;
}

void string_list_copy_to_array(char** str_array, struct StringList * sl) {
    u32  i = 0;
    for (struct StringListLink * sll = sl->first; sll ; sll=sll->next) {
        assert(i<sl->count);
        str_array[i++] = sll->str;
    }
    str_array[i] = 0;
}

usz const config_memory_SIZE = 1<<11;
char config_memory1[config_memory_SIZE];
char config_memory2[config_memory_SIZE];
char * _config_memory;
char * config_memory_end;
char * config_memory_next;


void config_initialize() {
    // Swap config memory each time we initialize
    if (_config_memory != config_memory1) {
        _config_memory = config_memory1;
    } else {
        _config_memory = config_memory2;
    }
    config_memory_next = _config_memory;
    config_memory_end = _config_memory + config_memory_SIZE;
}

void config_memory_copy(char** var) {
    if (*var) {
        if (_config_memory <= *var && *var < config_memory_end) {
        } else {
            *var = config_push_string(*var);
        }
    }
}

u8 config_memory_dirty;

// Lets set this up so that when we re-initialize, we swap between two config memories, and then
//  we copy all old memory into new memory
// TODO: We never reset config memeory between loading configs... we'd run out of space...

void * config_push(usz len, usz alignment) {
    if (config_memory_dirty == 0) {
        config_memory_dirty = 1;
        config_initialize();
    } else {
        assert(config_memory_dirty == 1);
    }

    ssz v = (ssz)config_memory_next; v += (-v) & (alignment -1);
    char* ret = config_memory_next = (char*)v;
    config_memory_next += len;
    if (config_memory_next > config_memory_end) {
        ERROR("Config: Out of memory");
        ret = 0;
    }
#if BUILD_IS_RELEASE == 0
    ret = malloc(len); // For debugging, enables ASAN
#endif
    return ret;
}

char* config_push_string(char * str) {
    usz len = strlen(str) + 1;
    char* ret = config_push(len, 1);
    memcpy(ret, str, len);
    return ret;
}

#if CONFIG_DIAGNOSTICS == 1
#define DIAG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#elif CONFIG_DIAGNOSTICS == 0
#define DIAG(fmt, ...) WARN(fmt, ##__VA_ARGS__)
#else
#error CONFIG_DIAGNOSTICS not set
#endif


#define _config_salt()  ({ *end = 0; if(config_user_adder) { config_salt(start); } return; })
#define _config_salt_old()  ({ *end = 0; if(config_user_adder) { config_salt_old(start); } return; })
#define _config_user_adder()  ({ *end = 0; if(config_user_adder) { config_user_adder(start); } return; })
#define _config_user_extender() ({ *end = 0; if(config_user_extender) { config_user_extender(start); } return; })
#define _config_user_normal(n1) ({ *end = 0; if(config_user_normal) { config_user_normal(start, n1); } return; })
#define BASE10(part) ({ *part ## _end = 0; strtol(part, 0, 10); })

#define set_config2(var, value) ({ if(__set_config2(#var, &var, line_number)) { var =  value; }; return; })
static u8 __set_config2(char* var_name, void * var, int line_number) {
    DEBUG("%s=%s", var_name, value);
    if (!var) {
        DEBUG("Ignoring config for variable: %s", var_name);
        return 0;
    } else {
        return 1;
    }
}


#define set_config(var) *end = 0; __set_config(#var, &var, start, line_number); return;
static void __set_config(char* var_name, char** var, char* value, int line_number) {
#if CONFIG_DIAGNOSTICS == 1
    if (*var) {
        printf("Line:%d Config value for '%s' is already set\n",
            line_number, var_name);
    }
    *var = config_push_string(value);
#elif CONFIG_DIAGNOSTICS == 0
    DEBUG("%s=%s", var_name, value);
    if (!var) {
        DEBUG("Ignoring config for variable: %s", var_name);
    } else {
        if (*var) {
            WARN("Config value for '%s' is already set, overwriting", var_name);
        }
        *var = config_push_string(value);
    }
#else
#error CONFIG_DIAGNOSTICS not set
#endif
}

#define config_append(list, val) __config_append(& list, val)
static void __config_append(struct StringList *sl, char* str) {
    string_list_append(sl, config_push_string(str),
     config_push(sizeof(struct StringListLink), _Alignof(struct StringListLink))
    );
}


#if CONFIG_DIAGNOSTICS == 1
char *   valid_config_user_adder[] = {
    "UserAdder: 8129933d4568c229f34a7a29869918e2ace401766f3701ba3e05da69f994382b",
00};
char *   valid_config_user_extender[] = {
    "UserExtender: 8129933d4568c229f34a7a29869918e2ace401766f3701ba3e05da69f994382b",
00};
char *   valid_config_user_normal[] = {
    "UserNormal: 49875 8129933d4568c229f34a7a29869918e2ace401766f3701ba3e05da69f994382b",
00};
char *   valid_config_email_from[] = {
    "EmailAddress:    asdasdfasdasd32323@gmail.com",
    "EmailAddress: \tasdasdfasdasd32323@gmail.com",
    "EmailAddress:    yahooyahoo@yahoo.com",
    "EmailAddress: tmp-from@testtest.test",
0};
char * invalid_config_email_from[] = {
    "EmailAddress:    asdasd@fasdasd32323@mail.com",
    "EmailAddress:    yahooyahoo@",
    "EmailAddress: @testtest.test",
    "EmailAddress: @",
    "EmailAddress: dasdfasdfasd",
0};

char *   valid_config_email_host[] = {
    "EmailServer:  smtps://smtp.gmail.com",
    "EmailServer:  smtps://smtp.gmail.com:465",
    "EmailServer:  smtp://127.0.0.1",
    "EmailServer:  smtp://127.0.0.1:8025",
    "EmailServer:  smtps://127.0.0.1:8025",
0};
char * invalid_config_email_host[] = {
    "EmailServer:  smtp.gmail.com",
    "EmailServer:  smtp://127.0.0.1:",
    "EmailServer:  smtpss://127.0.0.1:",
    "EmailServer:  smtpss://:333",
    "EmailServer:  smtps://127.0.0.1:8ddd025",
0};
char *   valid_config_email_rcpt[] = {
    "DestinationEmailAddress:    asdasdfasdasd32323@gmail.com",
    "DestinationEmailAddress:    yahooyahoo@yahoo.com",
    "DestinationEmailAddress: tmp-from@testtest.test",
0};
char * invalid_config_email_rcpt[] = {
    "DestinationEmailAddress:    asdasd@fasdasd32323@mail.com",
    "DestinationEmailAddress:    yahooyahoo@",
    "DestinationEmailAddress: @testtest.test",
    "DestinationEmailAddress: @",
    "DestinationEmailAddress: dasdfasdfasd",
0};
char *   valid_config_argv_config[] = {
    "DebugSupervisorArg: /bin/sh",
    "DebugSupervisorArg: -c",
    "DebugSupervisorArg: /usr/bin/ping 127.0.0.1 | ts",
0};
char *   valid_config_email_user_pass[] = {
    "EmailUserPass:  user:pass",
    "EmailUserPass:  314234123klj;k;asdfasd!~!@:@!#@#$%@#&^&!~8709870asdfaklj",
    "EmailUserPass:  :",
0};
char * invalid_config_email_user_pass[] = {
    "EmailUserPass: ",
    "EmailUserPass: adfasdfasdf",
    "EmailUserPass: 12341231234",
0};
char *   valid_config_day_sec_open[] = {
    "OpenAtSec:  4000",
0};
char * invalid_config_day_sec_open[] = {
    "OpenAtSec: ",
0};
char *   valid_config_day_sec_close[] = {
    "CloseAtSec:  4000",
0};
char * invalid_config_day_sec_close[] = {
    "CloseAtSec: ",
0};
char * valid_config_salt[] = {
    "Salt: 73616c747973616c7400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "Salt: 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
0};
char * invalid_config_salt[] = {
    "Salt: ",
    "Salt: 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "Salt: 0",
    "Salt: asdfasdf",
0};
char * valid_config_salt_old[] = {
    "SaltOld: 73616c747973616c7400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "SaltOld: 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
0};
char * invalid_config_salt_old[] = {
    "SaltOld: ",
    "SaltOld: 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "SaltOld: 0",
    "SaltOld: asdfasdf",
0};

#define do_diagnostic(long_string, short_var) *end=0; \
  __do_diagnostic(long_string, start, valid_config_ ## short_var, line_number); return;
static void __do_diagnostic(char * long_string, char * start, char** valid_examples, int line_number) {
    printf("On line:%d config value for %s is invalid:`%s'\n", line_number, long_string, start);
    printf("here are some valid examples:\n");
    for (char**ve = valid_examples; *ve; ve++) {
        printf("\t%s\n", *ve);
    }
}
#elif CONFIG_DIAGNOSTICS == 0
#define do_diagnostic(long_string, short_var) *end=0; __do_diagnostic(long_string, start, line_number); return;
static void __do_diagnostic(char * long_string, char * start, int line_number) {
        WARN("Line:%d Failed to validate: %s: '%s' please run config_validator", line_number, long_string, start);
}
#else
#error CONFIG_DIAGNOSTICS not set
#endif

#include "/build/config.re.c"
#undef  set_config
#undef  config_append
#undef  do_diagnostic


void config_load_file(char * file_path) {


    {
        memset(access_salt, 0, SALT_BUF_LEN);
        strcpy(access_salt, "saltysalt");
    }

    string_list_initialize(&tmp_arg);
    int r;
    int line_number = 1;
    FILE * f = fopen(file_path, "r"); error_check(f?0:-1);
    while (!feof(f)) {
        char buf[1024];
        char *buf_ptr = buf;
        usz start_len = sizeof buf;
        ssz len = getline(&buf_ptr, &start_len, f);
        if (len == -1) {
            if (!feof(f)) {
                error_check(len);
            }
            break;
        } if (len > start_len - 2) {
            printf("Error: Config line:%d too long\n", line_number);
        } else if (len > 0) {
            buf[len - 1] = 0; // remove newline
            log_allowed_fails = 100;
            config_parse_line(buf, line_number);
            log_allowed_fails = 0;
            line_number++;
        }
    }
    r = fclose(f); error_check(r);
}

/*
void config_dump_email_user_pass() {
    f("EmailUserPass: %s", email_user_pass);
}

*/
//void config_out_