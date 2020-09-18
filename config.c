
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"
#include "logging.h"
#include "config.h"

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
char config_memory[config_memory_SIZE];
char * const config_memory_end = config_memory + config_memory_SIZE;
char * config_memory_next = config_memory;


void config_initialize() {
    config_memory_next = config_memory;
}

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

#define set_config(var) *end = 0; __set_config(#var, &var, start, line_number, print_diagnostics); return;
static void __set_config(char* var_name, char** var, char* value, int line_number, u8 print_diagnostics) {
    if (!var) {
        DEBUG("Ignoring config for variable: %s", var_name);
    } else {
        if (*var) {
            if (print_diagnostics) {
                printf("Line:%d Config value for '%s' is already set, overwriting\n",
                    line_number, var_name);
            } else {
                WARN("Config value for '%s' is already set, overwriting", var_name);
            }
        }
        *var = config_push_string(value);
    }
}

#define config_append(list, val) __config_append(& list, val)
static void __config_append(struct StringList *sl, char* str) {
    string_list_append(sl, config_push_string(str),
     config_push(sizeof(struct StringListLink), _Alignof(struct StringListLink))
    );
}

#define do_diagnostic(long_string, short_var) *end=0; extern char * valid_config_ ## short_var[]; \
  __do_diagnostic(long_string, start, print_diagnostics, valid_config_ ## short_var, line_number); return;
static void __do_diagnostic(char * long_string, char * start, u8 print_diagnostics, char** valid_examples, int line_number) {
    if (print_diagnostics) {
        printf("On line:%d config value for %s is invalid:`%s'\n", line_number, long_string, start);
        printf("here are some valid examples:\n");
        for (char**ve = valid_examples; *ve; ve++) {
            printf("\t%s\n", *ve);
        }
    } else {
        WARN("Failed to validate: %s: '%s' please run config_validator", long_string, start);
    }
}

#include "/build/config.re.c"
#undef  set_config
#undef  config_append
#undef  do_diagnostic


void config_load_file(char * file_path) {
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
            config_parse_line(buf, 1, line_number);
            log_allowed_fails = 0;
            line_number++;
        }
    }
    r = fclose(f); error_check(r);
}

