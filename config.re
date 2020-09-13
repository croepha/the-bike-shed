
void parse_config(char *input_str) {
  char * YYMARKER, *YYCURSOR = input_str, * start = 0, * end = 0;

  /*!stags:re2c format = 'char *@@;'; */
  /*!re2c re2c:define:YYCTYPE = char; re2c:yyfill:enable = 0;

  *      { WARN("ignoring unknown config line: '%s'", input_str); return; }
  "EmailAddress:" [ ]* @start [-a-z0-9.+_]+ "@" [-a-z0-9.+_]+                 @end { *end=0; email_from=config_push_string(start); return;}
  "EmailServer:"  [ ]* @start "smtp" "s"? "://" [-a-z0-9.+_]+ ( ":" [0-9]+ )? @end { *end=0; email_host=config_push_string(start); return;}
  "EmailAddress:" [ ]* @start [^ \x00]* @end { *end=0; WARN("Failed to validate: EmailAddress '%s'", start); return; }
  "EmailServer:"  [ ]* @start [^ \x00]* @end { *end=0; WARN("Failed to validate: EmailServer: '%s'", start); return;}
  */


}
