#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

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

void serial_printf(char const *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void serial_got_char(char byte);
void pin_backlight_pwm_set(u32 value);
void got_keypad_input(char key);
void exterior_loop(void);
void exterior_setup(void);
u64 now_ms(void);

s32 rfid_id_scan(char*, s32);
static const unsigned int backlight_level_MAX = 32 * 4 * 256;

void _lcd_set_linenf(u8 line_i, char const *fmt, ...) __attribute__ ((format (printf, 2, 3)));
#define lcd_set_line0(fmt, ...) _lcd_set_linenf(0, fmt, ##__VA_ARGS__)
#define lcd_set_line1(fmt, ...) _lcd_set_linenf(1, fmt, ##__VA_ARGS__)
#define lcd_set_line2(fmt, ...) _lcd_set_linenf(2, fmt, ##__VA_ARGS__)
#define lcd_set_line3(fmt, ...) _lcd_set_linenf(3, fmt, ##__VA_ARGS__)





