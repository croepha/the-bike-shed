#pragma once

#include <sys/epoll.h>
#include "common.h"

typedef union {
  epoll_data_t data;
  struct {
    s32 id;
    u8 event_type;
  } my_data;
} io_EPData;


void io_process_events();


