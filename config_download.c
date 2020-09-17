#include <string.h>

#include "common.h"
#include "logging.h"
#include "config.h"

char config_download_leftover[config_download_leftover_SIZE];
usz config_download_leftover_used;

// It is likely that size is large, containing many lines
size_t config_download_write_callback(char *data, size_t size, size_t nmemb, void *userdata) {
  size = size * nmemb;
  for (;;) {
    char* nl = memchr(data, '\n', size);
    if (nl) {
      nl++;
      if (nl - data >
          config_download_leftover_SIZE - config_download_leftover_used) {
        ERROR("Line too long, throwing it out");
        size -= config_download_leftover_SIZE - config_download_leftover_used;
        data = nl;
        config_download_leftover_used = 0;
      } else {
        memcpy(config_download_leftover + config_download_leftover_used, data,
               nl - data);
        config_parse_line(config_download_leftover, 0, 0);
        size -= nl - data;
        data += nl - data;
        config_download_leftover_used = 0;
      }
    } else {
      if (config_download_leftover_used + size <
          config_download_leftover_SIZE) {
        memcpy(config_download_leftover + config_download_leftover_used, data,
               size);
        config_download_leftover_used += size;
      } else {
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
        config_download_leftover_used = config_download_leftover_SIZE;
      }
      break;
    }
  }
  return size;
}

char percon_config_etag[32];
u64  percon_config_modtime_sec;




// #include <inttypes.h>

// #include "io_curl.h"
// #include "/build/parse_headers.re.c"


// struct curl_slist* headers_list;


// static size_t _header_callback(char *buffer, size_t _s, size_t nitems, void *userdata) {
//   _WriteCtx * c = userdata;
//   buffer[nitems-2] = 0;
//   DEBUG("Got header: '%s'", buffer);
//   struct ParsedHeader header =  parse_header(buffer);
//   switch (header.type) { default: break;
//     case HEADER_TYPE_ETAG: {
//       DEBUG("HEADER_TYPE_ETAG %s\n", header.value);
//       size_t etag_len = strlen(header.value);
//       if (etag_len < sizeof c->etag) {
//         memcpy(c->etag, header.value, etag_len + 1);
//       }
//     } break;
//     case HEADER_TYPE_LAST_MODIFIED: {
//       u64 seconds = curl_getdate(header.value, 0);
//       DEBUG("HEADER_TYPE_LAST_MODIFIED %s %"PRId64"\n", header.value, seconds);
//       c->modified_time = seconds;
//     } break;
//   }
//   return nitems;
// }

