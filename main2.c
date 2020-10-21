

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "common.h"
#include "logging.h"
#include "config.h"
#include "io.h"
#include "access.h"
#include "email.h"

void serial_initialize(void);

int serial_fd = -1;


struct access_HashPayload access_hash_payload;

void gpio_pwm_start(void);


void serial_line(char* line);
void serial_line(char* line) { int r;

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
                __access_hash(result, &access_hash_payload);
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

void email_done(u8 is_error);
void email_done(u8 is_error) { int r;
    u16 idx = 0;
    u16 day = 0;
    if (is_error) {
        r = dprintf(serial_fd, "TEXT_SHOW Request failed day:%hu idx:%hu", idx, day);
        error_check(r);
    } else {
        r = dprintf(serial_fd, "TEXT_SHOW Request sent day:%hu idx:%hu", idx, day);
        error_check(r);
    }
}

void email_timeout(void);
void email_timeout() { int r;
    u16 idx = 0;
    u16 day = 0;
    r = dprintf(serial_fd, "TEXT_SHOW Request failed day:%hu idx:%hu", idx, day);
    error_check(r);
}


void serial_io_event(void);
void serial_io_event() {

    char buf[256];
    ssz buf_len = read(serial_fd, buf, sizeof buf);
    error_check(buf_len);

}

int main (int argc, char ** argv) {
    assert(argc == 2);
    setlinebuf(stderr);
    config_load_file(*++argv);


    char * serial_path = "/build/exterior_mock.pts";

    int serial_open(char* serial_path);
    serial_fd = serial_open(serial_path);

    // io_setup_reader(serial_fd);





}


