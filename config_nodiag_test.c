
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include "logging.h"
#include "config.h"

char * email_from;
char * email_host;
char * email_user_pass;
char * email_rcpt;
char ** supr_child_args;
struct StringList tmp_arg;



    // for (char * c=buf; *c; c++) { *c=tolower(*c); }


#define test_set(set, var) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set( set, COUNT(set), &var); }
static void _test_set(char**set, usz set_len, char** var) {
    for (int i=0; i < set_len; i++) {
        if (!set[i]) break;
        config_initialize();
        *var = 0;
        INFO("Trying line: '%s'", set[i]);
        char buf[1024];
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        config_parse_line(buf, 0);
        INFO("Effective: '%s' Failures: %d", *var, 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
}

#define test_set2(set) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set2( set, COUNT(set)); }
static void _test_set2(char**set, usz set_len) {
    config_initialize();
    string_list_initialize(&tmp_arg);
    for (int i=0; i < set_len; i++) {
        if (!set[i]) break;
        INFO("Trying line: '%s'", set[i]);
        char buf[1024];
        strcpy(buf, set[i]);
        log_allowed_fails = 100;
        config_parse_line(buf, 0);
        INFO("Failures: %d", 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
    for (struct StringListLink * sll = tmp_arg.first; sll ; sll= sll->next) {
        INFO("Effective: %s",sll->str);
    }
}


char *   valid_config_email_from[] = {
    "EmailAddress:    asdfsdfasdasd32323@GMAIL.com",
    "EmailAddress:    AASDFsdfasdasd32323@gmail.com",
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
    setlinebuf(stderr);
    setlinebuf(stdout);

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

    INFO("Trying out config_test1");
    {
        int r;
        FILE * f = fopen("/tmp/config_test1", "w"); error_check(f?0:-1);
        r = fputs(
            "adfa dfadkl;fa j;lsdkjfa;dj;\n"
            "EmailAddress:    yahooyahoo@yahoo.com\n"
            "DestinationEmailAddress: tmp-from@testtest.test\n"
            "DestinationEmailAddress:    asdasd@fasdasd32323@mail.com\n"
            "DestinationEmailAddress: tmp-from2@testtest.test\n"
            "DebugSupervisorArg: /bin/sh\n"
            "DebugSupervisorArg: -c\n"
            "DebugSupervisorArg: /usr/bin/ping 127.0.0.1 | ts\n"
        , f); error_check(r);
        r = fclose(f); error_check(r);
    }
    #define _(v) v = 0
    _(email_from);
    _(email_host);
    _(email_user_pass);
    _(email_rcpt);
    #undef  _
    supr_child_args = (char**){0};
    config_load_file("/tmp/config_test1");
    {
        INFO("Effective configs:");
        #define _(v) INFO("\t" #v ": `%s'", v)
        _(email_from);
        _(email_host);
        _(email_user_pass);
        _(email_rcpt);
        #undef  _
        supr_child_args = config_push((tmp_arg.count + 1) * sizeof(char*), _Alignof(char*));
        string_list_copy_to_array(supr_child_args, &tmp_arg);
        INFO("\tsupr_child_args:");
        for (char**c = supr_child_args; *c; c++) {
            INFO("\t\t'%s'", *c);
        }
    }
//char ** supr_child_args;


}