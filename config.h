#pragma once

#include "common.h"


struct StringListLink { struct StringListLink * next; char * str; };
struct StringList { struct StringListLink *first, **nextp; u32 count; };


void string_list_initialize(struct StringList * sl);
void string_list_copy_to_array(char** str_array, struct StringList * sl);
void string_list_append(struct StringList * sl, char * str, struct StringListLink * sll);

void config_initialize();
void * config_push(usz len, usz alignment);
char * config_push_string(char * str);
void config_load_file(char * file_path);
void config_parse_line(char *input_str, u8 print_diagnostics, int line_number);
size_t config_download_write_callback(char *data, size_t size, size_t nmemb, void *userdata);
static const usz config_download_leftover_SIZE = 16;

__attribute__((weak_import)) extern char * email_from;
__attribute__((weak_import)) extern char * email_host;
__attribute__((weak_import)) extern char * email_user_pass;
__attribute__((weak_import)) extern char * email_rcpt;
__attribute__((weak_import)) extern char ** supr_child_args;
__attribute__((weak_import)) extern struct StringList tmp_arg;


