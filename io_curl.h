
#pragma once

#include <curl/curl.h>

#include "common.h"

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

u8 io_curl_completed(CURL**easy, CURLcode*result, void*private);
CURL* io_curl_create_handle();
