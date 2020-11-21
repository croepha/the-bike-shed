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


void config_user_adder(char*) __attribute__((weak_import));
void config_user_extender(char*) __attribute__((weak_import));
void config_user_normal(char*, u16 expire_day) __attribute__((weak_import));

// Backup config variable from old memory to current memory
void config_memory_copy(char** var);
extern u8 config_memory_dirty;



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
extern char * serial_path  __attribute__((weak_import));
extern char * config_download_url  __attribute__((weak_import));
extern u32 shed_clear_timeout_ms  __attribute__((weak_import));
extern u32 shed_door_unlock_ms  __attribute__((weak_import));

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
