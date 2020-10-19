#include <string.h>

#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"
#include "config.h"
#include "access.h"
#include "email.h"

u64 now_ms() { return real_now_ms(); }

struct access_HashPayload access_hash_payload;
void gpio_pwm_start(void);

void gpio_pwm_start(void) {
    INFO();
}



static usz const option_LEN = 10;

char exterior_option [option_LEN];
char exterior_pin    [pin_LEN   ];
char exterior_rfid   [rfid_LEN  ];


static void __exterior_set(char * start, char * end, char * dest, char * dest_name, usz dest_size) {
  usz len = end - start;
  if (end - start >= dest_size - 1) {
    ERROR_BUFFER(start, len, " %s too long", dest_name);
  } else {
    memcpy(dest, start, len);
    dest[len] = 0;
  }
}
#define exterior_set(dest)  __exterior_set(start, end, dest, #dest, sizeof dest); return
#define clear(dest)  memset(dest, 0, sizeof dest)

static void exterior_scan_start() {
    clear(exterior_option);
    clear(exterior_pin   );
    clear(exterior_rfid  );
}

static void exterior_scan_finished() { int r;
    if (strcmp(exterior_option, "") == 0) { // Accesss request
        u8 granted = access_requested((char*)access_hash_payload.rfid, (char*)access_hash_payload.pin);
        if (granted) {
            r = dprintf(serial_fd, "TEXT_SHOW ACCESS GRANTED");
            error_check(r);
            gpio_pwm_start();
        } else {
            // TODO give reason?
            r = dprintf(serial_fd, "TEXT_SHOW ACCESS DENIED");
            error_check(r);
        }
    } else if (strcmp(exterior_option, "100") == 0) { // Email hash
        enum {
            STATE_IDLE,
            STATE_SENDING,
        } state = 0;
        u16 idx = 0;
        u16 day = 0;
        switch (state) {
            case STATE_IDLE: {
                r = dprintf(serial_fd, "TEXT_SHOW Request sending day:%hu idx:%hu", idx, day);
                error_check(r);
                access_HashResult result;
                access_hash(result, &access_hash_payload);
                struct email_Send;
                // email_init(&email_Send, ....)
            } break;
            case STATE_SENDING: {
                r = dprintf(serial_fd, "TEXT_SHOW Cant send, email busy");
                error_check(r);
            } break;
        }
    }
}

#include "/build/exterior_protocol.re.c"



// TODO needs maintenance
int main ()  {
    access_user_list_init();

}