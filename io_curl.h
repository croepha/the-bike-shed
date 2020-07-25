
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


u8 io_curl_completed(CURL**easy, CURLcode*result, void*private);
CURL* io_curl_create_handle();
