

void serial_line_handler(char *input_str) {
  char * YYMARKER, *YYCURSOR = input_str, * start = 0, * end = 0;

  /*!stags:re2c format = 'char *@@;'; */
  /*!re2c re2c:define:YYCTYPE = char; re2c:yyfill:enable = 0;

  *      { ERROR_BUFFER(input_str, strlen(input_str), "ignoring unknown exterior line:"); return; }

  "SCAN_START" { exterior_scan_start(); return; }
  "OPTION"     [ ]* @start [-a-zA-Z0-9.+_]+  @end { exterior_set(exterior_option); }
  "PIN"        [ ]* @start [-a-zA-Z0-9.+_]+  @end { exterior_set(exterior_pin   ); }
  "RFID"       [ ]* @start [a-fA-f0-9]+  @end { exterior_set(exterior_rfid_text  ); }
  "SCAN_FINISHED" { exterior_scan_finished(); return; }
  "EXTERIOR_RESTART" { exterior_restart(); return; }

  */

}
