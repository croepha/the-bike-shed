#define LOG_DEBUG
#include <string.h>

#include "common.h"
#include "io_curl.h"
#include "logging.h"
#include "io.h"
#include "serial.h"
#include "config.h"
#include "access.h"
#include "email.h"
#include "inttypes.h"
#include "config_download.h"
#include "pwm.h"


// TODO: Periodically write file for worstcase time recovery

u64 now_ms() { return real_now_ms(); }


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


SCAN_START
OPTION 200
PIN 123456
RFID 000102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED


SCAN_START
OPTION 100
PIN 123456
RFID f00102030405060708090a0b0c0d0e0f1011121314151617
SCAN_FINISHED


*/


#if BUILD_IS_RELEASE == 0
u8 log_trace_enabled = 0;
#endif

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
        if (!hex[2*i+0] || !hex[2*i+1]) {
            break;
        }
        buf[i] = (base16_to_int(hex[2*i+0]) << 4)
          | (base16_to_int(hex[2*i+1]) << 0);
    }
}

u8 add_next_user;

void shed_pwm_timeout(void) {
    gpio_pwm_set(0);
}


void clear_display_timeout() { int r;
    r = dprintf(serial_fd, "TEXT_SHOW1 \n");
    error_check(r);
    r = dprintf(serial_fd, "TEXT_SHOW2 \n");
    error_check(r);
}



static void exterior_scan_finished() { int r;

    u8 exterior_rfid[rfid_LEN] = {};
    buf_from_hex(exterior_rfid, sizeof exterior_rfid, exterior_rfid_text);

    if (add_next_user == 1) {
        // TODO enforce pin complexity?

        u16 days_left = -1;
        u8 granted = access_requested((char*)exterior_rfid, (char*)exterior_pin, &days_left);
        if (granted && days_left == (u16)-1) {
            r = dprintf(serial_fd, "TEXT_SHOW1 Cannot add existing\n");
            error_check(r);
            r = dprintf(serial_fd, "TEXT_SHOW2 philanthropist\n");
            error_check(r);
            IO_TIMER_MS(clear_display) = now_ms() + 2000;
        } else {
            access_HashResult hash = {};
            access_hash(hash, (char*)exterior_rfid, exterior_pin);
            access_user_add(hash, access_now_day() + 30);
            r = dprintf(serial_fd, "TEXT_SHOW1 User Added\n");
            error_check(r);
            r = dprintf(serial_fd, "TEXT_SHOW2 days_left:30\n");
            error_check(r);
            IO_TIMER_MS(clear_display) = now_ms() + 2000;
        }
        add_next_user = 0;

    } else if (strcmp(exterior_option, "") == 0) { // Accesss request
        u16 days_left = (u16)-1;
        u8 granted = access_requested((char*)exterior_rfid, (char*)exterior_pin, &days_left);
        if (granted) {
            r = dprintf(serial_fd, "TEXT_SHOW1 ACCESS GRANTED\n");
            error_check(r);
            r = dprintf(serial_fd, "TEXT_SHOW2 days_left:%hu\n", days_left);
            error_check(r);
            gpio_pwm_set(1);
            IO_TIMER_MS(shed_pwm) = now_ms() + 1000;
            IO_TIMER_MS(clear_display) = now_ms() + 2000;
        } else {
            // TODO give reason?
            if (days_left == 0) {
                r = dprintf(serial_fd, "TEXT_SHOW1 ACCESS DENIED\n");
                error_check(r);
                r = dprintf(serial_fd, "TEXT_SHOW2 Expired\n");
                error_check(r);
                IO_TIMER_MS(clear_display) = now_ms() + 2000;
            } else {
                r = dprintf(serial_fd, "TEXT_SHOW1 ACCESS DENIED\n");
                error_check(r);
                r = dprintf(serial_fd, "TEXT_SHOW2 Unknown User\n");
                error_check(r);
                IO_TIMER_MS(clear_display) = now_ms() + 2000;
            }
        }
    } else if (strcmp(exterior_option, "100") == 0) { // Email hash
        char * cancel_text = "";

        switch (state) {
            case STATE_SENDING_CANCELLED: {
                cancel_text = "C";
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
                r = dprintf(serial_fd, "TEXT_SHOW1 Request sending %s\n", cancel_text);
                error_check(r);
                r = dprintf(serial_fd, "TEXT_SHOW2 Day:%hu Idx:%hu\n", day, idx);
                error_check(r);
                IO_TIMER_MS(clear_display) = now_ms() + 2000;

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
                r = dprintf(serial_fd, "TEXT_SHOW2 Cant send, email busy, try again to cancel and send again\n");
                error_check(r);
                IO_TIMER_MS(clear_display) = now_ms() + 2000;
                state = STATE_SENDING_CANCELLED;
            } break;
        }
    } else if (strcmp(exterior_option, "200") == 0) {
        // Philanthripist sign up new user
        u16 days_left = (u16)-1;
        u8 granted = access_requested((char*)exterior_rfid, (char*)exterior_pin, &days_left);
        if (!granted) {
            r = dprintf(serial_fd, "TEXT_SHOW1 DENIED:\n");
            error_check(r);
            r = dprintf(serial_fd, "TEXT_SHOW2  Unknown User\n");
            error_check(r);
            IO_TIMER_MS(clear_display) = now_ms() + 2000;
        } else if (days_left != (u16)-1) {
            r = dprintf(serial_fd, "TEXT_SHOW1 DENIED: Not a\n");
            error_check(r);
            r = dprintf(serial_fd, "TEXT_SHOW2    philanthropist\n");
            error_check(r);
            IO_TIMER_MS(clear_display) = now_ms() + 2000;
        } else {
            add_next_user = 1;
            // TODO add timer to reset this
            r = dprintf(serial_fd, "TEXT_SHOW1 \n");
            error_check(r);
            r = dprintf(serial_fd, "TEXT_SHOW2 Will add next user\n");
            IO_TIMER_MS(clear_display) = now_ms() + 2000;
            error_check(r);
        }

    } else {
        r = dprintf(serial_fd, "TEXT_SHOW1 \n");
        error_check(r);
        r = dprintf(serial_fd, "TEXT_SHOW2 Unkown option\n");
        error_check(r);
        IO_TIMER_MS(clear_display) = now_ms() + 2000;
        ERROR("Got an unknown option from the exterior");
    }
}

static void exterior_restart() {
    int r;
    r = dprintf(serial_fd, "TEXT_SHOW1 Exterior restart  \n");
    error_check(r);
    r = dprintf(serial_fd, "TEXT_SHOW2 Detected  \n");
    error_check(r);
    IO_TIMER_MS(clear_display) = now_ms() + 2000;
    WARN("Exterior restart detected");
}



#define exterior_set(dest)  __exterior_set(start, end, dest, #dest, sizeof dest); return
#include "/build/exterior_protocol.re.c"


void emailed_hash_io_curl_complete(CURL *easy, CURLcode result, struct emailed_hash_CurlCtx * ctx) {

    email_free(&emailed_hash_email_ctx);
    state = STATE_IDLE;
}



//u64 config_download_interval_sec = 60 * 60; // 1 Hour
u64 config_download_interval_sec = 20;
//char * config_download_url = "http://127.0.0.1:9160/workspaces/the-bike-shed/shed_test_config";
char * config_download_url = "http://192.168.4.159:9160/workspaces/the-bike-shed/shed_test_config";

char config_download_previous_etag[32];
u64 config_download_previous_modified_time_sec;
static struct config_download_Ctx config_download_ctx;

static u64 last_config_download_sec = 0;

void config_download_finished(struct config_download_Ctx *c, u8 success) {
  memset(config_download_previous_etag, 0, sizeof config_download_previous_etag);
  strncpy(config_download_previous_etag, c->etag, sizeof config_download_previous_etag);
  IO_TIMER_MS(config_download) = (last_config_download_sec + config_download_interval_sec) * 1000;
}

void config_download_timeout() {
  DEBUG();
  config_download_abort(&config_download_ctx);
  last_config_download_sec = now_sec();
  memset(&config_download_ctx.line_accumulator_data, 0, sizeof config_download_ctx.line_accumulator_data);
  __config_download_start(
    &config_download_ctx,
    config_download_url,
    config_download_previous_etag,
    config_download_previous_modified_time_sec);

  IO_TIMER_MS(config_download) = (last_config_download_sec + config_download_interval_sec) * 1000;
}

void __debug_config_download_complete_hook() {};
struct StringList tmp_arg;

char * email_from = "shed-test@example.com";
//char * email_host = "smtp://127.0.0.1:8025";
char * email_host = "smtp://192.168.4.159:8025";
char * email_user_pass = "username:password";
char * email_rcpt = "shed-test-dest@example.com";
//char * serial_path = "/build/exterior_mock.pts";
char * serial_path = "/dev/ttyAMA0";

void shed_add_philantropist_hex(char* hex) {
    access_HashResult hash = {};
    buf_from_hex(hash, sizeof hash, hex);
    access_user_add(hash, -1);
}



//   access_idle_maintenance_prev = &access_users_first_idx;
//   while (*access_idle_maintenance_prev != (u16)-1) {
//     access_idle_maintenance();
//   }


u16 last_maintenance_day;
void idle_timeout() {
    // Lets do maintenance at 4AM PT, should probaby be the slowest time at NB
    u64 const ms_per_hour = 60 * 60 * 1000;
    u64 const ms_per_day  = 24 * ms_per_hour;

    u64 offset_ms = (7+4); // UTC to PT + 4 hours;
    u64 utc_ms = now_ms();
    u16 next_maint_day = ( (utc_ms - offset_ms + ms_per_day) / ms_per_day ) ;
    u16 maint_day = next_maint_day - 1;
    u64 next_maint_ms = next_maint_day * ms_per_day  + offset_ms;

    DEBUG("%"PRIu64" %hu", next_maint_ms, maint_day);

    if (*access_idle_maintenance_prev != (u16)-1) {
        access_idle_maintenance();
        io_idle_has_work = 1;
    } else if (maint_day != last_maintenance_day) {
        INFO("Maintenance Starting");
        last_maintenance_day = maint_day;
        access_idle_maintenance_prev = &access_users_first_idx;
        io_idle_has_work = 1;
    } else {
        INFO("Maintenance Finished");
        // Set timer to next day
        IO_TIMER_MS(idle) = next_maint_ms;
        io_idle_has_work = 0;
    }

//    last_maintenance_day
}


// TODO needs maintenance
int main ()  {
    setlinebuf(stderr);
    io_initialize();
    io_curl_initialize();
    access_user_list_init();
    serial_io_initialize(serial_path);
    config_download_timeout();
    idle_timeout();
    gpio_pwm_initialize();

    {
        int r;
        r = dprintf(serial_fd, "TEXT_SHOW1 System restart\n");
        error_check(r);
        r = dprintf(serial_fd, "TEXT_SHOW2 \n");
        error_check(r);
        IO_TIMER_MS(clear_display) = now_ms() + 2000;
    }

    assert(base16_to_int('0') == 0);
    assert(base16_to_int('1') == 1);
    assert(base16_to_int('9') == 9);
    assert(base16_to_int('a') == 10);
    assert(base16_to_int('f') == 15);
    assert(base16_to_int('A') == 10);
    assert(base16_to_int('F') == 15);

    for(;;) {
        log_allowed_fails = 100000000;
        io_process_events();
        io_curl_process_events();
    }
}