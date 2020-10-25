#pragma once

#include "common.h"

#include "line_accumulator.h"


struct StringListLink { struct StringListLink * next; char * str; };
struct StringList { struct StringListLink *first, **nextp; u32 count; };


void string_list_initialize(struct StringList * sl);
void string_list_copy_to_array(char** str_array, struct StringList * sl);
void string_list_append(struct StringList * sl, char * str, struct StringListLink * sll);

void config_initialize(void);
void * config_push(usz len, usz alignment);
char * config_push_string(char * str);
void config_load_file(char * file_path);


#if CONFIG_DIAGNOSTICS == 1
#else
#endif


void config_parse_line(char *input_str, int line_number);


static const usz config_download_leftover_SIZE = line_accumulator_Data_SIZE;

extern char * email_from;
extern char * email_host;
extern char * email_user_pass;
extern char * email_rcpt;
extern char ** supr_child_args;
extern struct StringList tmp_arg;
// __attribute__((weak_import))

extern char *   valid_config_email_from[];
extern char * invalid_config_email_from[];
extern char *   valid_config_email_host[];
extern char * invalid_config_email_host[];
extern char *   valid_config_email_rcpt[];
extern char * invalid_config_email_rcpt[];
extern char *   valid_config_argv_config[];
extern char *   valid_config_email_user_pass[];
extern char * invalid_config_email_user_pass[];
