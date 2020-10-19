

static usz const option_LEN = 10;

u8 exterior_option [option_LEN];
u8 exterior_pin    [pin_LEN   ];
u8 exterior_rfid   [rfid_LEN  ];


static void exterior_line_handler(char *input_str) {
  char * YYMARKER, *YYCURSOR = input_str, * start = 0, * end = 0;

  /*!stags:re2c format = 'char *@@;'; */
  /*!re2c re2c:define:YYCTYPE = char; re2c:yyfill:enable = 0;

  *      { ERROR_BUFFER(input_str, strlen(input_str), "ignoring unknown exterior line:"); return; }

  "SCAN_START" {
    exterior_option[0] = 0;
    exterior_pin   [0] = 0;
    exterior_rfid  [0] = 0;
    return;
  }

  "OPTION"     [ ]* @start [-a-z0-9.+_]+ "@" [-a-z0-9.+_]+                 @end { set_config(email_from); }


  "EmailAddress:"            [ ]* @start [-a-z0-9.+_]+ "@" [-a-z0-9.+_]+                 @end { set_config(email_from); }
  "DestinationEmailAddress:" [ ]* @start [-a-z0-9.+_]+ "@" [-a-z0-9.+_]+                 @end { set_config(email_rcpt); }
  "EmailServer:"             [ ]* @start "smtp" "s"? "://" [-a-z0-9.+_]+ ( ":" [0-9]+ )? @end { set_config(email_host); }
  "EmailUserPass:"           [ ]* @start [^ :\x00]* ":" [^ \x00]*                       @end { set_config(email_user_pass); }
  "DebugSupervisorArg:"      [ ]* @start  [^\x00]*                          @end { *end=0; config_append(tmp_arg, start); return; }
  "EmailAddress:"            [ ]* @start [^ \x00]* @end { do_diagnostic("EmailAddress"             , email_from ); }
  "DestinationEmailAddress:" [ ]* @start [^ \x00]* @end { do_diagnostic("DestinationEmailAddress"  , email_rcpt ); }
  "EmailServer:"             [ ]* @start [^ \x00]* @end { do_diagnostic("EmailServer"              , email_host ); }
  "EmailUserPass:"           [ ]* @start [^ \x00]* @end { do_diagnostic("EmailUserPass"            , email_user_pass ); }

  */

}
