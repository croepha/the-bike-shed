
void parse_config(char *input_str) {
  char * YYMARKER, *YYCURSOR = input_str, * start = 0, * end = 0;

  /*!stags:re2c format = 'char *@@;'; */
  /*!re2c re2c:define:YYCTYPE = char; re2c:yyfill:enable = 0;

  *      { WARN("ignoring unknown config line: '%s'", input_str); return; }
  "EmailAddress:" [ ]* @start [-a-z0-9.+_]+ "@" [-a-z0-9.+_]+                 @end { set_config(email_from); }
  "EmailServer:"  [ ]* @start "smtp" "s"? "://" [-a-z0-9.+_]+ ( ":" [0-9]+ )? @end { set_config(email_host); }
  "DebugSupervisorArg:" [ ]* @start [^\x00]* @end { *end=0; config_append(tmp_arg, start); return; }
  "EmailAddress:" [ ]* @start [^ \x00]* @end { *end=0; WARN("Failed to validate: EmailAddress '%s'", start); return; }
  "EmailServer:"  [ ]* @start [^ \x00]* @end { *end=0; WARN("Failed to validate: EmailServer: '%s'", start); return; }
  */


}
