#pragma once

#include "io_curl.h"
#include "line_accumulator.h"

struct config_download_Ctx {
  struct line_accumulator_Data line_accumulator_data;
  CURL *easy;
  int id;
  struct curl_slist* headers_list;
  char etag[32];
  u64  modified_time_sec;
  enum _io_curl_type curl_type;
};

void __config_download_start(struct config_download_Ctx *c, char *url, char *previous_etag,
               u64 previous_mod_time_sec);


void __debug_config_download_complete_hook(void);
void config_download_finished(struct config_download_Ctx *c, u8 success) ;
void config_download_abort(struct config_download_Ctx *c);
