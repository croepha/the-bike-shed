#include <string.h>

#include "common.h"
#include "logging.h"

void handle_line(char* line);


const usz leftover_SIZE = 16;
char leftover[leftover_SIZE];
usz  leftover_used;

// It is likely that size is large, containing many lines
void handle_data(char* data, usz size) {
  if (leftover_used) { // Rare case, resuming with some leftover data
    char* endp = memchr(data, '\n', size);
    usz next = -1;
    if (endp) {
      *endp = 0;
      next = (endp-data)+1;
    }

    if (leftover_used + next > leftover_SIZE) {
      ERROR("Line too long, throwing it out");
      if (endp) {
        leftover_used = 0;
        data += next;
        size -= next;
      } else {
        leftover_used = leftover_SIZE;
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
        return;
      }
    } else {
      memcpy(leftover + leftover_used, data, next);
      handle_line(leftover);
      data += next;
      size -= next;
    }
  }

  for(;;) {
    char* endp = memchr(data, '\n', size);
    usz next = -1;
    if (endp) {
      *endp = 0;
      next = (endp-data)+1;
      handle_line(data);
      data += next;
      size -= next;
    } else {
      if (size >= leftover_SIZE) {
        leftover_used = leftover_SIZE;
        ERROR("Leftover data would oveflow, size:%ld, throwing out the data", size);
        return;
      } else {
        memcpy(leftover, data, size);
        leftover_used = size;
        DEBUG("leftover size:%ld\n", size);
        return;
      }
    }
  }
}



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

