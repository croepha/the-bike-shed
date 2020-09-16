
#include <string.h>
#include <ctype.h>
#include "logging.h"

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
    return ret;
}

char* config_push_string(char * str) {
    usz len = strlen(str) + 1;
    char* ret = config_push(len, 1);
    memcpy(ret, str, len);
    return ret;
}

struct StringListLink { struct StringListLink * next; char * str; };
struct StringList { struct StringListLink *first, **nextp; u32 count; };

void string_list_initialize(struct StringList * sl) {
    sl->first = 0; sl->nextp = &sl->first; sl->count = 0;
}

void string_list_append(struct StringList*sl, char * str) {
        struct StringListLink * s = *sl->nextp = config_push(sizeof(struct StringListLink), _Alignof(struct StringListLink));
        sl->nextp = &(*sl->nextp)->next;
        sl->count++;
        s->next = 0;
        s->str = str;
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
    string_list_append(sl, config_push_string(str) );
}

struct StringList tmp_arg;

#include "/build/config.re.c"
#undef  set_config
#undef  config_append

    // for (char * c=buf; *c; c++) { *c=tolower(*c); }


#define test_set(set, var) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set( set, COUNT(set), &var); }
void _test_set(char**set, usz set_len, char** var) {
    for (int i=0; i < set_len; i++) {
        config_memory_next = config_memory;
        *var = 0;
        INFO("Trying line: '%s'", set[i]);
        char buf[1024];
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        parse_config(buf);
        INFO("Effective: '%s' Failures: %d", *var, 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
}

#define test_set2(set) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set2( set, COUNT(set)); }
void _test_set2(char**set, usz set_len) {
    config_memory_next = config_memory;
    string_list_initialize(&tmp_arg);
    for (int i=0; i < set_len; i++) {
        INFO("Trying line: '%s'", set[i]);
        char buf[1024];
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        parse_config(buf);
        INFO("Failures: %d", 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
    for (struct StringListLink * s = tmp_arg.first; s ; s= s->next) {
        INFO("Effective: %s",s->str);
    }
}


char *   valid_email_address[] = {
    "EmailAddress:    asdasdfasdasd32323@gmail.com",
    "EmailAddress:    yahooyahoo@yahoo.com",
    "EmailAddress: tmp-from@testtest.test",
};
char * invalid_email_address[] = {
    "EmailAddress:    asdasd@fasdasd32323@mail.com",
    "EmailAddress:    yahooyahoo@",
    "EmailAddress: @testtest.test",
    "EmailAddress: @",
    "EmailAddress: dasdfasdfasd",
};
char *   valid_email_server[] = {
    "EmailServer:  smtps://smtp.gmail.com",
    "EmailServer:  smtps://smtp.gmail.com:465",
    "EmailServer:  smtp://127.0.0.1",
    "EmailServer:  smtp://127.0.0.1:8025",
    "EmailServer:  smtps://127.0.0.1:8025",
};
char * invalid_email_server[] = {
    "EmailServer:  smtp.gmail.com",
    "EmailServer:  smtp://127.0.0.1:",
    "EmailServer:  smtpss://127.0.0.1:",
    "EmailServer:  smtpss://:333",
    "EmailServer:  smtps://127.0.0.1:8ddd025",
};
char *   valid_email_rcpt[] = {
    "DestinationEmailAddress:    asdasdfasdasd32323@gmail.com",
    "DestinationEmailAddress:    yahooyahoo@yahoo.com",
    "DestinationEmailAddress: tmp-from@testtest.test",
};
char * invalid_email_rcpt[] = {
    "DestinationEmailAddress:    asdasd@fasdasd32323@mail.com",
    "DestinationEmailAddress:    yahooyahoo@",
    "DestinationEmailAddress: @testtest.test",
    "DestinationEmailAddress: @",
    "DestinationEmailAddress: dasdfasdfasd",
};
char *   valid_argv_config[] = {
    "DebugSupervisorArg: /bin/sh",
    "DebugSupervisorArg: -c",
    "DebugSupervisorArg: /usr/bin/ping 127.0.0.1 | ts",
};
char *   valid_email_user_pass[] = {
    "EmailUserPass:  user:pass",
    "EmailUserPass:  314234123klj;k;asdfasd!~!@:@!#@#$%@#&^&!~8709870asdfaklj",
    "EmailUserPass:  :",
};
char * invalid_email_user_pass[] = {
    "EmailUserPass: ",
    "EmailUserPass: adfasdfasdf",
    "EmailUserPass: 12341231234",
};
//char *

// void array_from_string_list() {

// }

int main () {

    test_set(  valid_email_address, email_from);
    test_set(invalid_email_address, email_from);

    test_set(  valid_email_server, email_host);
    test_set(invalid_email_server, email_host);

    test_set(  valid_email_user_pass, email_user_pass);
    test_set(invalid_email_user_pass, email_user_pass);

    test_set(  valid_email_rcpt, email_rcpt);
    test_set(invalid_email_rcpt, email_rcpt);

    test_set2(valid_argv_config);
    tmp_arg.count++;
    supr_child_args = config_push(tmp_arg.count * sizeof(char*), _Alignof(char*));
    {
        u16  i = 0;
        for (struct StringListLink * s = tmp_arg.first; s ; s= s->next) {
            assert(i<tmp_arg.count);
            supr_child_args[i++] = s->str;
        }
        supr_child_args[i] = 0;
    }
    INFO("supr_child_args:");
    for (char**c = supr_child_args; *c; c++) {
        INFO("\t'%s'", *c);
    }







}