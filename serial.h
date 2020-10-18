#pragma once

int serial_open_115200_8n1(char const * path);
void serial_line_handler(char* line);
void serial_io_initialize(char* dev_path);
extern int serial_fd;

