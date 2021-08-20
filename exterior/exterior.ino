
#include <SPI.h>
#include <MFRC522.h>
#include <HardwareSerial.h>
#include <LiquidCrystal.h>


//#define LCD_DB4_GPIO   15
//#define KP_R0_GPIO     15
//
//#define LCD_DB5_GPIO    2
//#define KP_R1_GPIO      2
//
//#define LCD_DB6_GPIO    0
//#define KP_R2_GPIO      0
//
//#define LCD_DB7_GPIO    4
//#define KP_R3_GPIO      4
//
//#define KP_C0_GPIO     27
//#define KP_C1_GPIO     26
//#define KP_C2_GPIO     25
//#define KP_C3_GPIO     33
//
//#define LCD_E_GPIO     13
//#define LCD_RS_GPIO    14
//#define LCD_LED_GPIO    5
//
//#define RFID_RSI_GPIO  22
//#define RFID_MISO_GPIO 19
//#define RFID_MOSI_GPIO 23
//#define RFID_SCK_GPIO  18
//#define RFID_SDA_GPIO  21
//
//#define SERIAL_RX_GPIO 16 // RO
//#define SERIAL_TX_GPIO 17 // DI
//



#define LCD_DB4_GPIO   15
#define KP_R0_GPIO     15

#define LCD_DB5_GPIO    2
#define KP_R1_GPIO      2

#define LCD_DB6_GPIO    0
#define KP_R2_GPIO      0

#define LCD_DB7_GPIO    4
#define KP_R3_GPIO      4

#define KP_C0_GPIO     34
#define KP_C1_GPIO     35
#define KP_C2_GPIO     36
#define KP_C3_GPIO     39

#define LCD_E_GPIO     13
#define LCD_RS_GPIO    14
#define LCD_LED_GPIO    5

#define RFID_RSI_GPIO  22
#define RFID_MISO_GPIO 19
#define RFID_MOSI_GPIO 23
#define RFID_SCK_GPIO  18
#define RFID_SDA_GPIO  21

#define SERIAL_RX_GPIO 16
#define SERIAL_TX_GPIO 17
#define SERIAL_EN_GPIO 12

#define SPK_GPIO       25

#define LCD_LED_CHAN    0
#define SPK_CHAN        1


typedef  uint8_t   u8;
typedef  uint16_t u16;
typedef  uint32_t u32;
typedef  uint64_t u64;
typedef   int8_t   s8;
typedef   int16_t s16;
typedef   int32_t s32;
typedef   int64_t s64;
typedef  ssize_t  ssz;
typedef   size_t  usz;


#define col_pins _(KP_C0_GPIO) _(KP_C1_GPIO) _(KP_C2_GPIO) _(KP_C3_GPIO)
#define row_pins _(KP_R0_GPIO) _(KP_R1_GPIO) _(KP_R2_GPIO) _(KP_R3_GPIO)


volatile int pressed_col = -1;

#define _(n) if (pin == n) { return ret; } else { ret++; }
int row_i(int pin) { int ret = 0; row_pins }
int col_i(int pin) { int ret = 0; col_pins }
#undef _


int notes[] = {
  220, // A3
  247, // B3
  262, // C4
  294, // D4
  329, // E4
  349, // F4
  392, // G4
  440, // A4
  494, // B4
  523, // C5
  587, // D5
  659, // E5
  698, // F5
  783, // G5
  880, // A5
};

LiquidCrystal lcd(LCD_RS_GPIO, LCD_E_GPIO, LCD_DB4_GPIO, LCD_DB5_GPIO, LCD_DB6_GPIO, LCD_DB7_GPIO);

#define _(n) void IRAM_ATTR col ## n ## _isr() { pressed_col = n; }
col_pins
#undef _


MFRC522 mfrc522(RFID_SDA_GPIO, RFID_RSI_GPIO);  // Create MFRC522 instance
HardwareSerial InteriorSerial(1);


void setup_keypad() {
#define _(n) pinMode(n, OUTPUT); digitalWrite(n, LOW);
  row_pins
#undef _
#define _(n) pinMode(n, INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(n), col ## n ## _isr, FALLING);
  col_pins
#undef _
}

#define ERROR(...) Serial.printf("ERROR " __VA_ARGS__); Serial.printf("\n")

static const u32 line_accumulator_Data_SIZE = 512;

struct line_accumulator_Data {
  usz used;
  char data[line_accumulator_Data_SIZE];
};

long till_autosleep_ms = 0;

void line_accumulator(struct line_accumulator_Data * leftover, char* data, usz data_len, void(*line_handler)(char*)) {
  for (;;) {
    if (!data_len) break;
    char c = *data++;
    data_len--;
    if (c == '\n') {
      if (leftover->used == line_accumulator_Data_SIZE) {
        ERROR("Line too long, throwing it out");
        leftover->used = 0;
      } else {
        leftover->data[leftover->used] = 0;
        line_handler(leftover->data);
        leftover->used = 0;
      }
    } else if (leftover->used != line_accumulator_Data_SIZE) {
      if (leftover->used >= line_accumulator_Data_SIZE - 1) {
        ERROR("Couldn't find end of line, and out of space, throw out continued first line in next data set");
        leftover->used = line_accumulator_Data_SIZE;
      } else {
        leftover->data[leftover->used++] = c;
      }
    }
  }
}



struct line_accumulator_Data interior_line;


void setup() {
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(SERIAL_EN_GPIO, OUTPUT); digitalWrite(SERIAL_EN_GPIO, HIGH);


  Serial.begin(115200);
  Serial.println("INFO Started UP");
  InteriorSerial.begin(115200, SERIAL_8N1, SERIAL_RX_GPIO, SERIAL_TX_GPIO);
  InteriorSerial.println("INFO Started UP");


  lcd.begin(20, 4);
  setup_keypad();

	delay(4);				// Optional delay. Some board do need more time after init to be ready, see Readme
	mfrc522.PCD_DumpVersionToSerial();

  ledcSetup(LCD_LED_CHAN, 5000, 16);
  ledcAttachPin(LCD_LED_GPIO, LCD_LED_CHAN);


  ledcSetup(SPK_CHAN, 5000, 16);
  ledcAttachPin(SPK_GPIO, SPK_CHAN);
  ledcWrite(SPK_GPIO, 0);

  InteriorSerial.printf("EXTERIOR_RESTART\n");

}

void pullup_rows() {
#define _(n) pinMode(n, INPUT_PULLUP);
  row_pins
#undef _
}


unsigned int backlight_level = 0;
unsigned int backlight_level_MAX = 32 * 4 * 256;

void got_interior_line(char*line) {

  if (strncmp(line, "TEXT_SHOW1 ", 11) == 0) {
    #define _(n) pinMode(n, OUTPUT);
    row_pins
    #undef _
    lcd.setCursor(0,2);
    lcd.printf("%20s", line+11);
    setup_keypad();
    delay(10);
    pressed_col = -1;
    backlight_level = backlight_level_MAX;
    till_autosleep_ms = 5000;
  } else if (strncmp(line, "TEXT_SHOW2 ", 11) == 0) {
    #define _(n) pinMode(n, OUTPUT);
    row_pins
    #undef _
    lcd.setCursor(0,3);
    lcd.printf("%20s", line+11);
    setup_keypad();
    delay(10);
    pressed_col = -1;
    backlight_level = backlight_level_MAX;
    till_autosleep_ms = 5000;
  } else if (strncmp(line, "SLEEP", 5) == 0) {
    till_autosleep_ms = 0;
  } else {
    Serial.printf("got_interior_line unkown %s\n", line);
  }
}

u8 entering_option = 0;
char entered_pin[11];
int  entered_pin_used = 0;
char entered_option[4];
int  entered_option_used = 0;

void reset_input() {
    entering_option = 0;
    entered_pin_used = 0;
    entered_option_used = 0;
    memset(entered_pin, 0, sizeof entered_pin);
    memset(entered_option, 0, sizeof entered_option);

    #define _(n) pinMode(n, OUTPUT);
    row_pins
    #undef _
    lcd.setCursor(0,0);
    lcd.printf("                    ");
    lcd.setCursor(0,1);
    lcd.printf("                    ");
    setup_keypad();
    delay(10);
}

void draw_input_lines() {
  #define _(n) pinMode(n, OUTPUT);
  row_pins
  #undef _
  lcd.setCursor(0,0);
  if (0) {
    // Show pin
    lcd.printf("PIN:%12s%c", entered_pin, entering_option?' ':'<');
  } else {
    // Hide pin
    char buf[24];
    for (int i = 0; i < entered_pin_used; i++) {
      buf[i] = '*';
    }
    buf[entered_pin_used] = 0;
    lcd.printf("PIN:%12s%c", buf, entering_option?' ':'<');
  }
  lcd.setCursor(0,1);
  if (entered_option_used || entering_option) {
    lcd.printf("OPT:%12s%c", entered_option, entering_option?'<':' ');
  } else {
    lcd.printf("                   ");
  }
  setup_keypad();
  delay(10);
  pressed_col = -1;

}


void got_keypad_input(char key) {
  till_autosleep_ms = 5000;
  backlight_level = backlight_level_MAX;

  if (key == '*') {
    reset_input();
  } else if (entering_option) {
    if (key == '#') {
      entering_option = 0;
    } else {
      entered_option[entered_option_used++] = key;
      if (entered_option_used >= 3) {
        entering_option = 0;
      }
    }
  } else {
    if (key == '#') {
      entering_option = 1;
      if (entered_option[0]) {
        memset(entered_option, 0, sizeof entered_option);
        entered_option_used = 0;
      }
    } else {
      if (entered_pin_used < 10) {
        entered_pin[entered_pin_used++] = key;
      }
    }
  }
  draw_input_lines();

}



unsigned long last_uptime_ms = 0;

void loop() {

  unsigned long uptime_delta_ms_old = last_uptime_ms;
  last_uptime_ms = millis();
  long uptime_delta_ms = last_uptime_ms - uptime_delta_ms_old;
  if (uptime_delta_ms > 10000) { uptime_delta_ms = 10; } // Overflow or reset...
  if (till_autosleep_ms > 0) {
    till_autosleep_ms -= uptime_delta_ms;
    if (till_autosleep_ms <= 0) {
       till_autosleep_ms = 0;
       reset_input();
    }
  }

  ledcWrite(LCD_LED_CHAN, backlight_level);
  if (till_autosleep_ms <= 0 && backlight_level > 0) {
    unsigned long delta_level = (backlight_level_MAX / 1000) * uptime_delta_ms;
    if (delta_level > backlight_level) {
      backlight_level = 0;
    } else {
      backlight_level -= delta_level;
    }
  }

  int tone_key = -1;

  delay(1);
  if (pressed_col != -1) {
    int hit_col = pressed_col;

    int pressed_row = -1;

#define _(n) pullup_rows(); pinMode(n, OUTPUT); digitalWrite(n, LOW); \
    if (!digitalRead(pressed_col)) { pressed_row = n; goto scan_done; }
    row_pins
#undef _

    scan_done:

    int hits = 0;
    if (pressed_row != -1) {
      for (int i = 0; i < 20; i++) {
        delay(1);
        if (hit_col != pressed_col) {break;}
        if (digitalRead(hit_col)) {
          break;
        }
        hits++;
      }
      tone_key = row_i(pressed_row)*4+col_i(hit_col);
      char c = "123A456B789C*0#D"[row_i(pressed_row)*4+col_i(hit_col)];
      if (hits == 20) {

        Serial.printf("pressed1 %c row:%d col:%d hits:%d\n", c, pressed_row, hit_col, hits);
        got_keypad_input(c);

      }
    }
    pressed_col = -1;
    //Serial.printf("pressed row:%d col:%d hits:%d\n", pressed_row, hit_col, hits);
  }
//  static int _i = 0;
//  if (_i++ > 100) {
//    _i = 0;
//  //#define _(n)  detachInterrupt(digitalPinToInterrupt(n)); pinMode(n, INPUT);
//  //    col_pins
//  //#undef _
//  #define _(n) pinMode(n, OUTPUT);
//    row_pins
//  #undef _
//      Serial.printf("poo\n");
//      lcd.printf("33333333333333333333333333333333333333333333333333333333333333333333ABC");
//      pressed_col = -1;
//  }
  setup_keypad();

  if (0) {
    if (tone_key == -1) {
        //ledcWrite(SPK_CHAN, 0);
    } else {
        ledcWrite(SPK_CHAN, 32 * 4 * 256);
        ledcWriteTone(SPK_CHAN, notes[tone_key]);
    }
  }

  while (InteriorSerial.available() > 0) {
    char in_byte = InteriorSerial.read();
    if (in_byte == -1) break;

    line_accumulator(&interior_line, &in_byte, 1, got_interior_line);
  }

  if (entered_pin_used &&
      mfrc522.PICC_IsNewCardPresent() &&
      mfrc522.PICC_ReadCardSerial() ) {

      char rfid_str[21];
      for (int i = 0; i< mfrc522.uid.size; i ++ ) {
        sprintf(rfid_str + i*2, "%02x", mfrc522.uid.uidByte[i]);
      }

      Serial.printf("RFID %s\n", rfid_str);

      InteriorSerial.printf("SCAN_START\n");
      if (entered_option_used) {
        InteriorSerial.printf("OPTION %s\n", entered_option);
      }
      InteriorSerial.printf("PIN %s\n", entered_pin);
      InteriorSerial.printf("RFID %s\n", rfid_str);
      InteriorSerial.printf("SCAN_FINISHED\n");

      reset_input();

  }




	// Dump debug info about the card; PICC_HaltA() is automatically called
	//mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}
