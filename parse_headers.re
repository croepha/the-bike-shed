

enum HEADER_TYPE {
  HEADER_TYPE_INVALID,
  HEADER_TYPE_OTHER,
  HEADER_TYPE_LAST_MODIFIED,
  HEADER_TYPE_ETAG,
};

struct ParsedHeader {
  int type;
  char * value;
};

static struct ParsedHeader parse_header(char *YYCURSOR) {
  char * YYMARKER, * start = 0, * end;
  struct ParsedHeader ret = {HEADER_TYPE_INVALID, 0};

  #define RET(m_type) ({ ret.type = HEADER_TYPE_ ## m_type; ret.value = start; return ret; })

  /*!stags:re2c format = 'char *@@;'; */

  /*!re2c
  re2c:define:YYCTYPE = char;
  re2c:yyfill:enable = 0;
  re2c:flags:tags = 1;

  *      { RET(OTHER); }
  [\x00] { RET(INVALID); }
  "Last-Modified:" [ ]* @start { RET(LAST_MODIFIED); }
  "ETag:" [ ]* ["] @start [^\x00]* @end ["] { *end = 0; RET(ETAG); }

  */
}
