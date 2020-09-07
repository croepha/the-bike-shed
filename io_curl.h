
#pragma once

#include <curl/curl.h>

#include "common.h"

#define _IO_CURL_TYPES \
  _(test) \
  _(logging) \
  _(config_download) \
  _(supervisor_email) \

#define _(name) _io_curl_type_ ## name,
enum _io_curl_type { _(INVALID) _IO_CURL_TYPES _(COUNT)};
#undef _


#define _(name) void __ ## name ## _io_curl_complete(CURL* easy, CURLcode result, enum _io_curl_type * private) __attribute__((weak_import));
_IO_CURL_TYPES
#undef  _


#define IO_CURL_SETUP(name, struct_name, member_name) \
static void name ## _io_curl_complete(CURL *easy, CURLcode result, struct_name *c); \
void __ ## name ## _io_curl_complete(CURL *easy, CURLcode result, enum _io_curl_type *private) { \
  name ## _io_curl_complete(easy, result, (struct_name*)((u8*)private - __builtin_offsetof(struct_name, member_name))); } \
static CURL* name ## _io_curl_create_handle(struct_name *c) { \
  c->member_name = _io_curl_type_ ## name; \
  return __io_curl_create_handle(&c-> member_name); }




#define CURLCHECK(_m_curl_code) ({ \
  if (_m_curl_code != CURLE_OK) { \
    ERROR("CURLCODE:%s", curl_easy_strerror(_m_curl_code)); } \
})

#define CURLESET(m_key, ...) ({ \
  CURLcode _m_curl_code = curl_easy_setopt( \
    easy, CURLOPT_ ## m_key, ##__VA_ARGS__); \
  if (_m_curl_code != CURLE_OK) { \
    ERROR("curl_easy_setopt(CURLOPT_" #m_key ", " #__VA_ARGS__"): %s", \
    curl_easy_strerror(_m_curl_code)); } \
})

__attribute__((unused))
static char const * _error_check_CURLcode(CURLcode c) {
  if (c == CURLE_OK) { return 0; }
  return curl_easy_strerror(c);
}

__attribute__((unused))
static char const * _error_check_CURLMcode(CURLMcode c) {
  if (c == CURLM_OK) { return 0; }
  return curl_multi_strerror(c);
}

__attribute__((unused))
static char const * _error_check_CURLSHcode(CURLSHcode c) {
  if (c == CURLSHE_OK) { return 0; }
  return curl_share_strerror(c);
}

#define error_check_curl(  err) ({ char const * error_check_s = _error_check_CURLcode  (err); if (error_check_s) { ERROR("Error   CURLcode:%d:%s", err, error_check_s); } })
#define error_check_curlm( err) ({ char const * error_check_s = _error_check_CURLMcode (err); if (error_check_s) { ERROR("Error  CURLMcode:%d:%s", err, error_check_s); } })
#define error_check_curlsh(err) ({ char const * error_check_s = _error_check_CURLSHcode(err); if (error_check_s) { ERROR("Error CURLSHcode:%d:%s", err, error_check_s); } })

void io_curl_abort(CURL* easy);
CURL* __io_curl_create_handle();
void io_curl_process_events();
void io_curl_initialize();

