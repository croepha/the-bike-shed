#include <string.h>

#include "common.h"
#include "logging.h"
#include "config.h"



static void __line_handler(char* line) {
  config_parse_line(line, 0);
}


struct line_accumulator_Data leftover_d;

// It is likely that size is large, containing many lines
size_t config_download_write_callback(char *data, size_t size, size_t nmemb, void *userdata) {
  size = size * nmemb;
  line_accumulator(&leftover_d, data, size, __line_handler);
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

