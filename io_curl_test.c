
// TODO: Write test for timeout on really slow url
#define LOG_DEBUG
#include <stdlib.h>
#include <string.h>

// internal:
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <curl/curl.h>


#include <inttypes.h>
#include "common.h"
#include "logging.h"
#include "io.h"
#include "io_curl.h"
#include "config.h"
#include "config_download.h"

#include <openssl/sha.h>





void config_parse_line() {}
u64 now_ms() { return real_now_ms(); }

void line_accumulator(struct line_accumulator_Data *leftover, char *data, usz data_len, void (*line_handler)(char *)) {
  assert(sizeof(SHA256_CTX) < sizeof(struct line_accumulator_Data));
  SHA256_Update((SHA256_CTX*)leftover, data, data_len);
}



void config_download_finished(struct config_download_Ctx *c, u8 success) {
  if (success) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, (SHA256_CTX*)&c->line_accumulator_data);
    INFO_HEXBUFFER(hash, SHA256_DIGEST_LENGTH);
  }
}


int pending_events;
void __debug_config_download_complete_hook(void) {
  pending_events --;
  log_allowed_fails = 100;
}

static void dl(struct config_download_Ctx *c, char *url, char *previous_etag,
               u64 previous_mod_time_sec) {
  assert(sizeof(SHA256_CTX) < sizeof(struct line_accumulator_Data));
  SHA256_Init((SHA256_CTX*)&c->line_accumulator_data);
  __config_download_start(c, url, previous_etag, previous_mod_time_sec);
  pending_events ++;
}

static void _perform_all() {
  while (pending_events > 0) {
    io_process_events();
    io_curl_process_events();
  }
}




static void download_test() {

  setbuf(stdout, 0);
  setbuf(stderr, 0);

  io_initialize();
  io_curl_initialize();

  struct config_download_Ctx c1 = {.id = 1};
  char* url = "http://127.0.0.1:9160/workspaces/the-bike-shed/README.md";
  char* url2 = "http://127.0.0.1:9161/workspaces/the-bike-shed/README.md";
  dl(&c1, url, 0, 0);

  struct config_download_Ctx c2 = {.id = 2};
  dl(&c2, "ftp://127.0.0.1:232/asdfas", 0, 0);

  struct config_download_Ctx c3 = {.id = 3};
  dl(&c3, url2, 0, 0);

  _perform_all();

  struct config_download_Ctx c4 = {.id = 4};
  dl(&c4, url, c1.etag, c1.modified_time_sec);

  struct config_download_Ctx c5 = {.id = 5};
  dl(&c5, url, 0, 0);

  struct config_download_Ctx c6 = {.id = 6};
  dl(&c6, url2, 0, 0);

  _perform_all();

}

int main() {
  download_test();
}
