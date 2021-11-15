
#include <SPI.h>
#include <MFRC522.h>
#include <HardwareSerial.h>
#include <LiquidCrystal.h>
#include <cstdarg>
#include <stdarg.h>
#include <stdio.h>

#define MIN(a,b) (a < b ? a : b)


//#define LCD_DB4_GPIO   15
//#define KP_R0_GPIO     15
//
//#define LCD_DB5_GPIO
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

extern "C" {
#include "exterior.h"
}

extern "C" u64 now_ms() { return millis(); }
extern "C" void pin_backlight_pwm_set(u32 value) {
    ledcWrite(LCD_LED_CHAN, value);
}


#define col_pins _(KP_C0_GPIO) _(KP_C1_GPIO) _(KP_C2_GPIO) _(KP_C3_GPIO)
#define row_pins _(KP_R0_GPIO) _(KP_R1_GPIO) _(KP_R2_GPIO) _(KP_R3_GPIO)


volatile int pressed_col = -1;

#define _(n) if (pin == n) { return ret; } else { ret++; }
static int row_i(int pin) { int ret = 0; row_pins }
static int col_i(int pin) { int ret = 0; col_pins }
#undef _


// Bass-ed
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

void serial_printf(const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  char buf[1024];
  vsnprintf(buf, sizeof buf, fmt, va);
  InteriorSerial.print(buf);
  va_end(va);
}

extern "C" void _log_lite(const char* severity, const char*file, const char*func, int line, char* fmt, ...) {
  char log_buffer[1024];
  va_list va; va_start(va, fmt);
  int r = snprintf(log_buffer, sizeof log_buffer, fmt, va);
  va_end(va);
  assert(r < sizeof log_buffer);
  u64 now_ms_ = now_ms();
  fprintf(stderr, "%06"PRIx64".%03"PRIu64": %s: %s ", now_ms_ / 1000, now_ms_ % 1000, severity, log_buffer);
}


void setup() {
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(SERIAL_EN_GPIO, OUTPUT); digitalWrite(SERIAL_EN_GPIO, HIGH);


  Serial.begin(115200);
  Serial.println("INFO Started UP");
  InteriorSerial.begin(115200, SERIAL_8N1, SERIAL_RX_GPIO, SERIAL_TX_GPIO);

  lcd.begin(20, 4);
  setup_keypad();

	delay(4);				// Optional delay. Some board do need more time after init to be ready, see Readme
	mfrc522.PCD_DumpVersionToSerial();

  ledcSetup(LCD_LED_CHAN, 5000, 16);
  ledcAttachPin(LCD_LED_GPIO, LCD_LED_CHAN);


  ledcSetup(SPK_CHAN, 5000, 16);
  ledcAttachPin(SPK_GPIO, SPK_CHAN);
  ledcWrite(SPK_GPIO, 0);

  exterior_setup();

}

void pullup_rows() {
#define _(n) pinMode(n, INPUT_PULLUP);
  row_pins
#undef _
}

// void _lcd_set_linenf(u8 line_i, const char *fmt, ...) {
//   lcd.setCursor(0,line_i);
//   va_list va;
//   va_start(va, fmt);
//   char buf[1024];
//   vsnprintf(buf, sizeof buf, fmt, va);
//   va_end(va);
//   lcd.print(buf);
//   setup_keypad();
//   delay(10);
//   pressed_col = -1;
// }


int rfid_id_scan(char * dest, s32 size) {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    assert(size >= mfrc522.uid.size);
    size = MIN(size, mfrc522.uid.size);
    memcpy(dest, mfrc522.uid.uidByte, size);
    return size;
  } else {
    return 0;
  }
}

void loop() {
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
      char c = "123A456B789C*0#D"[row_i(pressed_row)*4+col_i(hit_col)];
      if (hits == 20) {

        Serial.printf("pressed1 %c row:%d col:%d hits:%d\n", c, pressed_row, hit_col, hits);
        got_keypad_input(c);

      }
    }
    pressed_col = -1;
    //Serial.printf("pressed row:%d col:%d hits:%d\n", pressed_row, hit_col, hits);
  }

  setup_keypad();

  while (InteriorSerial.available() > 0) {
    char in_byte = InteriorSerial.read();
    if (in_byte == -1) break;
    serial_got_char(in_byte);
  }

  exterior_loop();

}
