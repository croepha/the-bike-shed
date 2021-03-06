


void config_parse_line(char *input_str, int line_number) {
  LOGCTX(" parse");

  char * YYMARKER, *YYCURSOR = input_str, * start = 0, * end = 0;

  char * part1 = 0, * part1_end = 0;
  // char * s1 = 0, * e1 = 0, * s2 = 0, * e2 = 0;

  /*!stags:re2c format = 'char *@@;'; */
  /*!re2c re2c:define:YYCTYPE = char; re2c:yyfill:enable = 0;

  *      { DIAG("ignoring unknown config line: '%s'", input_str); return; }
  "EmailAddress:"            [ \t]* @start [-a-zA-Z0-9.+_]+ "@" [-a-zA-Z0-9.+_]+                 @end { set_config(email_from); }
  "DestinationEmailAddress:" [ \t]* @start [-a-zA-Z0-9.+_]+ "@" [-a-zA-Z0-9.+_]+                 @end { set_config(email_rcpt); }
  "EmailServer:"             [ \t]* @start "smtp" "s"? "://" [-a-zA-Z0-9.+_]+ ( ":" [0-9]+ )?    @end { set_config(email_host); }
  "EmailUserPass:"           [ \t]* @start [^ :\x00]* ":" [^ \x00]*                              @end { set_config(email_user_pass); }
  "ConfigURL:"               [ \t]* @start [^ :\x00]* ":" [^ \x00]*                              @end { set_config(config_download_url); }
  "DebugSupervisorArg:"      [ \t]* @start  [^\x00]*                          @end { *end=0; config_append(tmp_arg, start); return; }
  "DebugSerialPath:"         [ \t]* @start  [^\x00]*                          @end { set_config(serial_path); }
  "DebugClearTimeoutMS:"     [ \t]* @part1 [0-9]+ @part1_end          { set_config2(shed_clear_timeout_ms, BASE10(part1)); }
  "ConfigDownloadStartupDelayMS:" [ \t]* @part1 [0-9]+ @part1_end          { set_config2(config_download_startup_delay_ms, BASE10(part1)); }
  "DoorUnlockMS:"            [ \t]* @part1 [0-9]+ @part1_end          { set_config2(shed_door_unlock_ms, BASE10(part1)); }
  "UserAdder:"               [ \t]* @start [0-9a-fA-F]{64} @end { _config_user_adder(); }
  "UserExtender:"            [ \t]* @start [0-9a-fA-F]{64} @end { _config_user_extender(); }
  "UserNormal:"              [ \t]* @part1 [0-9]+ @part1_end [ \t]+ @start [0-9a-fA-F]{64} @end { _config_user_normal(BASE10(part1)); }
  "OpenAtSec:"               [ \t]* @part1 [0-9]+ @part1_end          { set_config2(day_sec_open, BASE10(part1)); }
  "CloseAtSec:"              [ \t]* @part1 [0-9]+ @part1_end          { set_config2(day_sec_close, BASE10(part1)); }
  "Salt:"                    [ \t]* @start [0-9a-fA-F]{128} @end { _config_salt(); }
  "SaltOld:"                 [ \t]* @start [0-9a-fA-F]{128} @end { _config_salt_old(); }
  "EmailAddress:"            [ \t]* @start [^ \x00]* @end { do_diagnostic("EmailAddress"             , email_from ); }
  "DestinationEmailAddress:" [ \t]* @start [^ \x00]* @end { do_diagnostic("DestinationEmailAddress"  , email_rcpt ); }
  "EmailServer:"             [ \t]* @start [^ \x00]* @end { do_diagnostic("EmailServer"              , email_host ); }
  "EmailUserPass:"           [ \t]* @start [^ \x00]* @end { do_diagnostic("EmailUserPass"            , email_user_pass ); }
  "UserAdder:"               [ \t]* @start [^ \x00]* @end { do_diagnostic("UserAdder"                , user_adder ); }
  "UserExtender:"            [ \t]* @start [^ \x00]* @end { do_diagnostic("UserExtender"             , user_extender ); }
  "UserNormal:"              [ \t]* @start [^ \x00]* @end { do_diagnostic("UserNormal"               , user_normal ); }
  "OpenAtSec:"               [ \t]* @start [^ \x00]* @end { do_diagnostic("OpenAtSec"                , day_sec_open ); }
  "CloseAtSec:"              [ \t]* @start [^ \x00]* @end { do_diagnostic("CloseAtSec"               , day_sec_close ); }
  "Salt:"                    [ \t]* @start [^ \x00]* @end { do_diagnostic("Salt"                     , salt ); }
  "SaltOld:"                 [ \t]* @start [^ \x00]* @end { do_diagnostic("SaltOld"                  , salt_old ); }

  */

}
