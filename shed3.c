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

static void exterior_scan_finished() { int r;

    u8 exterior_rfid[rfid_LEN] = {};
    buf_from_hex(exterior_rfid, sizeof exterior_rfid, exterior_rfid_text);

    if (strcmp(exterior_option, "") == 0) { // Accesss request
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
                struct access_HashPayload access_hash_payload;
                __access_hash(h, &access_hash_payload);
                r = snprintf(emailed_hash_buf, sizeof emailed_hash_buf,
                    "Day: %hu Idx: %hu\n"
                    "Hash: "
                    "%016"PRIx64"%016"PRIx64"%016"PRIx64"%016"PRIx64""
                    "%016"PRIx64"%016"PRIx64"%016"PRIx64"%016"PRIx64"\n",
                    day, idx,
                    *(u64*)&h[ 0], *(u64*)&h[ 8], *(u64*)&h[16], *(u64*)&h[24],
                    *(u64*)&h[32], *(u64*)&h[40], *(u64*)&h[48], *(u64*)&h[56]
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

    access_HashResult hash = {};
    buf_from_hex(hash, sizeof hash, "859f3efa83dc0b75985995516c06d05fd7516e06b4b318ecbc63adde14a945e21e038bc5b5e1c5eb19b88ffa73d954b8149f50c7d4fa8ae92e86d60c8af2e785");
    access_user_add(hash, access_now_day() + 1);

    for(;;) {
        log_allowed_fails = 100000000;
        io_process_events();
        io_curl_process_events();
    }
}