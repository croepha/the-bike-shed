
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

//void shed_add_philantropist_hex(char* hex) {}


#define test_set(set, var) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set( set, &var); }
static void _test_set(char**set, char** var) {
    for (;*set; set++) {
        if (!*set) break;
        config_initialize();
        *var = 0;
        INFO("Trying line: '%s'", *set);
        char buf[1024];
        strcpy(buf, *set);
        log_allowed_fails = 100;
        config_parse_line(buf, 0);
        INFO("Effective: '%s' Failures: %d", *var, 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
}

#define test_set2(set) INFO("Testing set: %s", #set); { LOGCTX("\t"); _test_set2( set); }
static void _test_set2(char**set) {
    config_initialize();
    string_list_initialize(&tmp_arg);
    for (;*set; set++) {
        if (!*set) break;
        INFO("Trying line: '%s'", *set);
        char buf[1024];
        strcpy(buf, *set);
        log_allowed_fails = 100;
        config_parse_line(buf, 0);
        INFO("Failures: %d", 100 - log_allowed_fails);
        log_allowed_fails = 0;
    }
    for (struct StringListLink * sll = tmp_arg.first; sll ; sll= sll->next) {
        INFO("Effective: %s",sll->str);
    }
}


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
            "UserAdder: 8129933d4568c229f34a7a29869918e2ace401766f3701ba3e05da69f994382b\n"
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