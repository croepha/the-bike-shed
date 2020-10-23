#include <string.h>

#include "common.h"
#include "logging.h"
#include "io.h"
#include "serial.h"
#include "config.h"
#include "access.h"
#include "email.h"
#include "inttypes.h"

u64 now_ms() { return real_now_ms(); }

void gpio_pwm_start(void);

void gpio_pwm_start(void) {
    INFO();
}


/*

SCAN_START
OPTION 001
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED

SCAN_START
OPTION 100
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED

SCAN_START
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED


*/

static usz const option_LEN = 10;

char exterior_option [option_LEN];
char exterior_pin    [pin_LEN   ];
char exterior_rfid_text [rfid_LEN * 2 + 1];

static void __exterior_set(char * start, char * end, char * dest, char * dest_name, usz dest_size) {
  usz len = end - start;
  if (len >= dest_size) {
    ERROR_BUFFER(start, len, "  %s too long, len:%zu/%zu :", dest_name, len,dest_size);
  } else {
    memcpy(dest, start, len);
    dest[len] = 0;
  }
}
#define clear(dest)  memset(dest, 0, sizeof dest)

static void exterior_scan_start() {
    clear(exterior_option);
    clear(exterior_pin   );
    clear(exterior_rfid_text);
}

u16 emailed_hash_idx = 0;
u16 emailed_hash_day = 0;
char emailed_hash_buf[2048];
usz  emailed_hash_buf_used = 0;
struct email_Send emailed_hash_email_ctx;
struct emailed_hash_CurlCtx {
  enum _io_curl_type curl_type;
};
struct emailed_hash_CurlCtx emailed_hash_curl_ctx;
IO_CURL_SETUP(emailed_hash, struct emailed_hash_CurlCtx, curl_type);

enum {
    STATE_IDLE,
    STATE_SENDING_NO_CANCEL,
    STATE_SENDING_CANCELLED,
} state = 0;

static int base16_to_int(char c) {
    if ('A' <= c && c <= 'F') {
        return 10 + c - 'A';
    } else if ('a' <= c && c <= 'f') {
        return 10 + c - 'a';
    } else if ('0' <= c && c <= '9') {
        return c - '0';
    } else {
        ERROR("bad char: %c", c);
        return 0;
    }
}

static void buf_from_hex(void* buf_, usz buf_len, char * hex) {
    u8 * buf = buf_;
    for (int i = 0; i < buf_len; i++) {
        buf[i] = (base16_to_int(hex[2*i+0]) << 4)
          | (base16_to_int(hex[2*i+1]) << 0);
    }
}

u8 add_next_user;

static void exterior_scan_finished() { int r;

    u8 exterior_rfid[rfid_LEN] = {};
    buf_from_hex(exterior_rfid, sizeof exterior_rfid, exterior_rfid_text);

    if (add_next_user == 1) {
        // TODO enforce pin complexity?
        access_HashResult hash = {};
        access_hash(hash, (char*)exterior_rfid, exterior_pin);
        access_user_add(hash, access_now_day() + 30);
        r = dprintf(serial_fd, "TEXT_SHOW User Added days_left:30\n");
        error_check(r);
        add_next_user = 0;
    } else if (strcmp(exterior_option, "") == 0) { // Accesss request
        u16 days_left = (u16)-1;
        u8 granted = access_requested((char*)exterior_rfid, (char*)exterior_pin, &days_left);
        if (granted) {
            r = dprintf(serial_fd, "TEXT_SHOW ACCESS GRANTED days_left:%hu\n", days_left);
            error_check(r);
            gpio_pwm_start();
        } else {
            // TODO give reason?
            if (days_left == 0) {
                r = dprintf(serial_fd, "TEXT_SHOW ACCESS DENIED: Expired\n");
                error_check(r);
            } else {
                r = dprintf(serial_fd, "TEXT_SHOW ACCESS DENIED: Unknown User\n");
                error_check(r);
            }
        }
    } else if (strcmp(exterior_option, "100") == 0) { // Email hash
        char * cancel_text = "";

        switch (state) {
            case STATE_SENDING_CANCELLED: {
                cancel_text = "Cancelled previous send";
                WARN("Cancelling previous email send");
                email_free(&emailed_hash_email_ctx); // Aborting previous send...
            } // Fall through
            case STATE_IDLE: {
                u16 day = access_now_day();
                if (day != emailed_hash_day) {
                    emailed_hash_day = day;
                    emailed_hash_idx = 0;
                }
                u16 idx = emailed_hash_idx++;
                r = dprintf(serial_fd, "TEXT_SHOW Request sending Day:%hu Idx:%hu %s\n", day, idx, cancel_text);
                error_check(r);

                access_HashResult h;
                access_hash(h, (char*)exterior_rfid, (char*)exterior_pin);
                INFO_HEXBUFFER(h, sizeof h, "Requested send   Day:%hu Idx:%hu :", day, idx);
                r = snprintf(emailed_hash_buf, sizeof emailed_hash_buf,
                    "Day: %hu Idx: %hu\n"
                    "Hash: "
                    "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x",
                    day, idx,
                    h[ 0], h[ 1], h[ 2], h[ 3], h[ 4], h[ 5], h[ 6], h[ 7],
                    h[ 8], h[ 9], h[10], h[11], h[12], h[13], h[14], h[15],
                    h[16], h[17], h[18], h[19], h[20], h[21], h[22], h[23],
                    h[24], h[25], h[26], h[27], h[28], h[29], h[30], h[31],
                    h[32], h[33], h[34], h[35], h[36], h[37], h[38], h[39],
                    h[40], h[41], h[42], h[43], h[44], h[45], h[46], h[47],
                    h[48], h[49], h[50], h[51], h[52], h[53], h[54], h[55],
                    h[56], h[57], h[58], h[59], h[60], h[61], h[62], h[63]
                    );
                if (r == -1 || r >= sizeof emailed_hash_buf - 1) {
                    ERROR("emailed_hash_buf snprintf r:%d", r);
                    emailed_hash_buf_used = 0;
                } else {
                    emailed_hash_buf_used = r;
                    email_init(
                        &emailed_hash_email_ctx,
                        emailed_hash_io_curl_create_handle(&emailed_hash_curl_ctx),
                        email_rcpt, emailed_hash_buf, emailed_hash_buf_used,
                        "Hash Send"
                    );
                    state = STATE_SENDING_NO_CANCEL;
                }
            } break;
            case STATE_SENDING_NO_CANCEL: {
                WARN("Got send request while email is pending");
                r = dprintf(serial_fd, "TEXT_SHOW Cant send, email busy, try again to cancel and send again\n");
                error_check(r);
                state = STATE_SENDING_CANCELLED;
            } break;
        }
    } else if (strcmp(exterior_option, "200") == 0) {
        // Philanthripist sign up new user
        u16 days_left = (u16)-1;
        u8 granted = access_requested((char*)exterior_rfid, (char*)exterior_pin, &days_left);
        if (!granted) {
            r = dprintf(serial_fd, "TEXT_SHOW DENIED: Unknown User\n");
            error_check(r);
        } if (days_left != (u16)-1) {
            r = dprintf(serial_fd, "TEXT_SHOW DENIED: Not a philanthrapist\n");
            error_check(r);
        } else {
            add_next_user = 1;
            // TODO add timer to reset this
            r = dprintf(serial_fd, "TEXT_SHOW Will add next user\n");
            error_check(r);
        }

    } else {
        r = dprintf(serial_fd, "TEXT_SHOW Unkown option\n");
        error_check(r);
        ERROR("Got an unknown option from the exterior");
    }
}

#define exterior_set(dest)  __exterior_set(start, end, dest, #dest, sizeof dest); return
#include "/build/exterior_protocol.re.c"


void emailed_hash_io_curl_complete(CURL *easy, CURLcode result, struct emailed_hash_CurlCtx * ctx) {
    email_free(&emailed_hash_email_ctx);
    state = STATE_IDLE;
}


char * email_from = "shed-test@example.com";
char * email_host = "smtp://127.0.0.1:8025";
char * email_user_pass = "username:password";
char * email_rcpt = "shed-test-dest@example.com";
char * serial_path = "/build/exterior_mock.pts";

// TODO needs maintenance
int main ()  {
    setlinebuf(stderr);
    io_initialize();
    io_curl_initialize();
    access_user_list_init();
    serial_io_initialize(serial_path);

    assert(base16_to_int('0') == 0);
    assert(base16_to_int('1') == 1);
    assert(base16_to_int('9') == 9);
    assert(base16_to_int('a') == 10);
    assert(base16_to_int('f') == 15);
    assert(base16_to_int('A') == 10);
    assert(base16_to_int('F') == 15);

    access_HashResult hash = {};
    buf_from_hex(hash, sizeof hash, "8129933d4568c229f34a7a29869918e2ace401766f3701ba3e05da69f994382b341c5d548ee9d9c2d8396f7b56198e3c6fc3c3951b57590fe996ebb4a303abed");
    access_user_add(hash, access_now_day() + 1);

    for(;;) {
        log_allowed_fails = 100000000;
        io_process_events();
        io_curl_process_events();
    }
}