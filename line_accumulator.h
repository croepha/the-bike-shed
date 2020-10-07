#pragma once

#include "common.h"

static const u32 line_accumulator_Data_SIZE = 512;

struct line_accumulator_Data {
  usz used;
  char data[line_accumulator_Data_SIZE];
};

void line_accumulator(struct line_accumulator_Data * leftover, char* data, usz data_len, void(*line_handler)(char*));
