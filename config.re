


void config_parse_line(char *input_str, int line_number) {

  char * YYMARKER, *YYCURSOR = input_str, * start = 0, * end = 0;

  // char * s1 = 0, * e1 = 0, * s2 = 0, * e2 = 0;

  /*!stags:re2c format = 'char *@@;'; */
  /*!re2c re2c:define:YYCTYPE = char; re2c:yyfill:enable = 0;

  *      { DIAG("ignoring unknown config line: '%s'", input_str); return; }
  "EmailAddress:"            [ \t]* @start [-a-zA-Z0-9.+_]+ "@" [-a-zA-Z0-9.+_]+                 @end { set_config(email_from); }
  "DestinationEmailAddress:" [ \t]* @start [-a-zA-Z0-9.+_]+ "@" [-a-zA-Z0-9.+_]+                 @end { set_config(email_rcpt); }
  "EmailServer:"             [ \t]* @start "smtp" "s"? "://" [-a-zA-Z0-9.+_]+ ( ":" [0-9]+ )?    @end { set_config(email_host); }
  "EmailUserPass:"           [ \t]* @start [^ :\x00]* ":" [^ \x00]*                              @end { set_config(email_user_pass); }
  "DebugSupervisorArg:"      [ \t]* @start  [^\x00]*                          @end { *end=0; config_append(tmp_arg, start); return; }
  "UserAdder:"               [ \t]* @start [0-9a-fA-F]{128} @end { config_user_adder(); }
  "EmailAddress:"            [ \t]* @start [^ \x00]* @end { do_diagnostic("EmailAddress"             , email_from ); }
  "DestinationEmailAddress:" [ \t]* @start [^ \x00]* @end { do_diagnostic("DestinationEmailAddress"  , email_rcpt ); }
  "EmailServer:"             [ \t]* @start [^ \x00]* @end { do_diagnostic("EmailServer"              , email_host ); }
  "EmailUserPass:"           [ \t]* @start [^ \x00]* @end { do_diagnostic("EmailUserPass"            , email_user_pass ); }
  "UserAdder:"               [ \t]* @start [^ \x00]* @end { do_diagnostic("UserAdder"                , user_adder ); }

  */

}
