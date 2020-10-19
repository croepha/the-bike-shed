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

void serial_line_handler(char* line) { int r;

    /* Line is something like:
    SCAN_START
    OPTION 001
    PIN  123123234
    RFID 12341234123412341234123412341234
    SCAN_FINISHED

    */

    // Parse line.... do different things...

    // SCAN_START
    // memset(access_hash_payload.salt, 0, salt...);
    memset(access_hash_payload.pin, 0, sizeof access_hash_payload.pin);
    memset(access_hash_payload.rfid, 0, sizeof access_hash_payload.rfid);
    char option[8];
    int  option_int = 0;
    memset(option, 0, sizeof option);


    // PIN
    strcpy((char*)access_hash_payload.pin, line);

    // RFID
    strcpy((char*)access_hash_payload.rfid, line);

    // OPTION
    strcpy((char*)option, line);

    // SCAN_FINISHED
    if (option_int == 0) { // Accesss request
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
    } else if (option_int == 100) { // Email hash
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

int main ()  {

}