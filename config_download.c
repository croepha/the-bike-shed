#include <string.h>

#include "common.h"
#include "logging.h"

void handle_line(char* line);


const usz leftover_SIZE = 16;
char leftover[leftover_SIZE];
usz  leftover_used;

// It is likely that size is large, containing many lines
void handle_data(char* data, usz size) {
  for (;;) {
    char* nl = memchr(data, '\n', size);
    if (nl) {
      nl++;
      if (nl - data > leftover_SIZE - leftover_used) {
        ERROR("Line too long, throwing it out");
        size -= leftover_SIZE - leftover_used;
        data = nl;
        leftover_used = 0;
      } else {
        memcpy(leftover + leftover_used, data, nl - data);
        handle_line(leftover);
        size -= nl - data;
        data += nl - data;
        leftover_used = 0;
      }
    } else {
      if (leftover_used + size < leftover_SIZE) {
        memcpy(leftover + leftover_used, data, size);
        leftover_used += size;
      } else {
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
        leftover_used = leftover_SIZE;
      }
      break;
    }
  }
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

