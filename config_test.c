
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "logging.h"

struct StringListLink { struct StringListLink * next; char * str; };
struct StringList { struct StringListLink *first, **nextp; u32 count; };

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

char * email_from;
char * email_host;
char * email_user_pass;
char * email_rcpt;
char ** supr_child_args;


usz const config_memory_SIZE = 1<<11;
char config_memory[config_memory_SIZE];
char * const config_memory_end = config_memory + config_memory_SIZE;
char * config_memory_next = config_memory;


void * config_push(usz len, usz alignment) {
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

#define set_config(var) *end = 0; __set_config(#var, &var, start); return;
static void __set_config(char* var_name, char** var, char* value) {
    if (*var) {
        WARN("Config value for '%s' is already set, overwriting", var_name);
    }
    *var = config_push_string(value);
}

#define config_append(list, val) __config_append(& list, val)
void __config_append(struct StringList *sl, char* str) {
    string_list_append(sl, config_push_string(str),
     config_push(sizeof(struct StringListLink), _Alignof(struct StringListLink))
    );
}

#define do_diagnostic(long_string, short_var) *end=0; extern char * valid_config_ ## short_var[]; \
  __do_diagnostic(long_string, start, print_diagnostics, valid_config_ ## short_var); return;
void __do_diagnostic(char * long_string, char * start, u8 print_diagnostics, char** valid_examples) {
    if (print_diagnostics) {
        printf("On line:%d config value for %s is invalid:`%s'", 0, long_string, start);
        printf("here are some valid examples:");
        for (char**ve = valid_examples; *ve; ve++) {
            printf("\t%s", *ve);
        }
    } else {
        WARN("Failed to validate: %s: '%s' please run config_validator", long_string, start);
    }
}

struct StringList tmp_arg;

#include "/build/config.re.c"
#undef  set_config
#undef  config_append
#undef  do_diagnostic

    // for (char * c=buf; *c; c++) { *c=tolower(*c); }


#define test_set(set, var) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set( set, COUNT(set), &var); }
void _test_set(char**set, usz set_len, char** var) {
    for (int i=0; i < set_len; i++) {
        if (!set[i]) break;
        config_memory_next = config_memory;
        *var = 0;
        INFO("Trying line: '%s'", set[i]);
        char buf[1024];
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        parse_config(buf, 0);
        INFO("Effective: '%s' Failures: %d", *var, 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
}

#define test_set2(set) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set2( set, COUNT(set)); }
void _test_set2(char**set, usz set_len) {
    config_memory_next = config_memory;
    string_list_initialize(&tmp_arg);
    for (int i=0; i < set_len; i++) {
        if (!set[i]) break;
        INFO("Trying line: '%s'", set[i]);
        char buf[1024];
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        parse_config(buf, 0);
        INFO("Failures: %d", 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
    for (struct StringListLink * sll = tmp_arg.first; sll ; sll= sll->next) {
        INFO("Effective: %s",sll->str);
    }
}


char *   valid_config_email_from[] = {
    "EmailAddress:    asdasdfasdasd32323@gmail.com",
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

int main () {

    test_set(  valid_config_email_from, email_from);
    test_set(invalid_config_email_from, email_from);

    test_set(  valid_config_email_host, email_host);
    test_set(invalid_config_email_host, email_host);

    test_set(  valid_config_email_user_pass, email_user_pass);
    test_set(invalid_config_email_user_pass, email_user_pass);

    test_set(  valid_config_email_rcpt, email_rcpt);
    test_set(invalid_config_email_rcpt, email_rcpt);

    test_set2(valid_config_argv_config);

    supr_child_args = config_push((tmp_arg.count + 1) * sizeof(char*), _Alignof(char*));
    string_list_copy_to_array(supr_child_args, &tmp_arg);
    INFO("supr_child_args:");
    for (char**c = supr_child_args; *c; c++) {
        INFO("\t'%s'", *c);
    }








}