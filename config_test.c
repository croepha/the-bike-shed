
#include <string.h>
#include <ctype.h>
#include "logging.h"

char * email_from;
char * email_host;
char * email_user_pass;
char * supr_email_rcpt;
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


static void __set_config(char* var_name, char** var, char* value) {
    if (*var) {
        WARN("Config value for '%s' is already set, overwriting", var_name);
    }
    *var = config_push_string(value);
}
#define set_config(var) *end = 0; __set_config(#var, &var, start); return;
#include "/build/config.re.c"
#undef  set_config

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
        config_memory_next = config_memory;
        email_host = 0;
        INFO("Trying line: '%s'", set[i]);
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        parse_config(buf);
        INFO("Effective: '%s' Failures: %d", email_host, 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
}

struct StringList { struct StringList * next; char * str; };
void append_string_list(struct StringList *** nextp, char * str) {
        struct StringList * s = **nextp = config_push(sizeof(struct StringList), _Alignof(struct StringList));
        *nextp = &(**nextp)->next;
        s->next = 0;
        s->str = str;
}

struct StringList *tmp_arg_first = 0, **tmp_arg_nextp = &tmp_arg_first;
u16  tmp_arg_count = 0;
void __config_append(u16 * count, struct StringList *** nextp, char* str) {
    append_string_list(nextp, config_push_string(str) );
    (*count)++;
}
#define config_append(list, val) __config_append(& list ## _count, & list ## _nextp, val)


#define test_set(set) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set( set, COUNT(set)); }
int main () {

    test_set(  valid_email_server);
    test_set(invalid_email_server);

    char buf[1024];

    strcpy(buf, valid_email_address0);  parse_config(buf);


    //email_host         = config_push_string("smtp://127.0.0.1:8025");
    email_user_pass    = config_push_string("user:pass");
    supr_email_rcpt    = config_push_string("logging@tmp-test.test");

    config_append(tmp_arg, "/bin/sh");
    config_append(tmp_arg, "-c");
    config_append(tmp_arg, "/usr/bin/ping 127.0.0.1 | ts");

    tmp_arg_count++;
    supr_child_args = config_push(tmp_arg_count * sizeof(char*), _Alignof(char*));
    {
        u16  i = 0;
        for (struct StringList * s = tmp_arg_first; s ; s= s->next) {
            assert(i<tmp_arg_count);
            supr_child_args[i++] = s->str;
        }
        supr_child_args[i] = 0;
    }

    // char* tmp_array[20];
    // char** t = tmp_array;
    // *t++ = config_push_string("/bin/sh");
    // *t++ = config_push_string("-c");
    // *t++ = config_push_string("/usr/bin/ping 127.0.0.1 | ts");
    // *t++ = 0;
    // supr_child_args = config_push((u8*)t - (u8*)tmp_array, _Alignof(u8*));
    // memcpy(supr_child_args, tmp_array, (u8*)t - (u8*)tmp_array);


    INFO("email_from: '%s'", email_from);
    INFO("email_host: '%s'", email_host);
    INFO("email_user_pass: '%s'", email_user_pass);
    INFO("supr_email_rcpt: '%s'", supr_email_rcpt);
    INFO("supr_child_args:");
    for (char**c = supr_child_args; *c; c++) {
        INFO("\t'%s'", *c);
    }


}