#define LOG_DEBUG
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

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

char * const serial_path_DEFAULT = "/dev/ttyAMA0";


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

u32 const shed_clear_timeout_ms_DEFAULT = 2000;
u32 shed_clear_timeout_ms = shed_clear_timeout_ms_DEFAULT;

u32 const shed_door_unlock_ms_DEFAULT = 1000;
u32 shed_door_unlock_ms = shed_door_unlock_ms_DEFAULT;


u32 const config_download_startup_delay_ms_DEFAULT = 5000;
u32 config_download_startup_delay_ms = config_download_startup_delay_ms_DEFAULT;


u32 day_sec_open;
u32 day_sec_close;

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

enum {
    add_user_state_NOT_ADDING,
    add_user_state_ADDING_NEW,
    add_user_state_EXTENDING,
} add_next_user;

void shed_pwm_timeout(void) {
    gpio_pwm_set(0);
}


void clear_display_timeout() { int r;
    r = dprintf(serial_fd, "TEXT_SHOW1 \n");
    error_check(r);
    r = dprintf(serial_fd, "TEXT_SHOW2 \n");
    error_check(r);
    r = dprintf(serial_fd, "SLEEP\n");
    error_check(r);
}

char* config_path;

#define config_backup_path_SET \
  char config_backup_path[64]; \
  __config_backup_path(config_backup_path, sizeof config_backup_path)
static void __config_backup_path(char* buf, usz size) {
    ssz r = snprintf(buf, size, "%s.backup", config_path);
    error_check(r);
    if (r >= size) {
        ERROR("Buf too small");
    }
}


__attribute__((__format__ (__printf__, 1, 2)))
static void exterior_display(char * fmt, ...) {
    va_list va;
    char buf[1024];
    va_start(va, fmt);
    int r = vsnprintf(buf, sizeof buf, fmt, va);
    va_end(va);
    error_check(r);
    char* nl = strchr(buf, '\n');
    char * line2 = "";
    if (nl) {
        *nl = 0;
        line2 = nl+1;
    }
    r = dprintf(serial_fd, "TEXT_SHOW1 %s\n", buf);
    error_check(r);
    r = dprintf(serial_fd, "TEXT_SHOW2 %s\n", line2);
    error_check(r);
    IO_TIMER_MS(clear_display) = now_ms() + shed_clear_timeout_ms;
}


static void save_config() {
    int r;

    config_backup_path_SET;

    if (!access(config_path, F_OK)) {
        r = rename(config_path, config_backup_path);
        error_check( r );
        sync();
    }

    u8 had_error = 0;

    int fd = open(config_path, O_WRONLY | O_CREAT);
    if (fd < 0) had_error = 1;
    error_check( fd );

    if (email_from) { r = dprintf(fd, "EmailAddress: %s\n", email_from); error_check(r); }
    if (email_host) { r = dprintf(fd, "EmailServer: %s\n", email_host); error_check(r); }
    if (email_user_pass) { r = dprintf(fd, "EmailUserPass: %s\n", email_user_pass); error_check(r); }
    if (email_rcpt) { r = dprintf(fd, "DestinationEmailAddress: %s\n", email_rcpt); error_check(r); }
    if (config_download_url) { r = dprintf(fd, "ConfigURL: %s\n", config_download_url); error_check(r); }
    if (serial_path != serial_path_DEFAULT) { r = dprintf(fd, "DebugSerialPath: %s\n", serial_path); error_check(r); }
    if (shed_clear_timeout_ms != shed_clear_timeout_ms_DEFAULT) { r = dprintf(fd, "DebugClearTimeoutMS: %d\n", shed_clear_timeout_ms); error_check(r); }
    if (shed_door_unlock_ms != shed_door_unlock_ms_DEFAULT) { r = dprintf(fd, "DoorUnlockMS: %d\n", shed_door_unlock_ms); error_check(r); }
    if (config_download_startup_delay_ms != config_download_startup_delay_ms_DEFAULT) { r = dprintf(fd, "ConfigDownloadStartupDelayMS: %d\n", config_download_startup_delay_ms); error_check(r); }
    r = dprintf(fd, "OpenAtSec: %u\n", day_sec_open); error_check(r);
    r = dprintf(fd, "CloseAtSec: %u\n", day_sec_close); error_check(r);

    #define USER (access_users_space[USER_idx])
    for (access_user_IDX USER_idx = access_users_first_idx; USER_idx != access_user_NOT_FOUND; USER_idx = USER.next_idx) {

        if (USER.expire_day == access_expire_day_magics_Adder   ||
            USER.expire_day == access_expire_day_magics_NewAdder) {
            r = dprintf(fd, "UserAdder: ");
            error_check(r);
        } else if (USER.expire_day == access_expire_day_magics_Extender   ||
                    USER.expire_day == access_expire_day_magics_NewExtender) {
            r = dprintf(fd, "UserExtender: ");
            error_check(r);
        } else {
            r = dprintf(fd, "UserNormal: %d ", USER.expire_day);
            error_check(r);
        }
        if (r < 0) had_error = 1;

    #define h (USER.hash)
        r = dprintf(fd,
            "%02x%02x%02x%02x%02x%02x%02x%02x"
            "%02x%02x%02x%02x%02x%02x%02x%02x"
            "%02x%02x%02x%02x%02x%02x%02x%02x"
            "%02x%02x%02x%02x%02x%02x%02x%02x\n",
            h[ 0], h[ 1], h[ 2], h[ 3], h[ 4], h[ 5], h[ 6], h[ 7],
            h[ 8], h[ 9], h[10], h[11], h[12], h[13], h[14], h[15],
            h[16], h[17], h[18], h[19], h[20], h[21], h[22], h[23],
            h[24], h[25], h[26], h[27], h[28], h[29], h[30], h[31]
            );
    #undef h
        error_check(r);
        if (r < 0) had_error = 1;
    }
    #undef USER
    r = close(fd);
    if (r < 0) had_error = 1;
    error_check(r);
    sync();

    if (!had_error) unlink(config_backup_path);

}


static void unlock_door() {
    gpio_pwm_set(1);
    //DEBUG("shed_door_unlock_ms:%u", shed_door_unlock_ms);
    IO_TIMER_MS(shed_pwm) = now_ms() + shed_door_unlock_ms;
}

static void exterior_scan_finished() { int r;

    u8 exterior_rfid[rfid_LEN] = {};
    buf_from_hex(exterior_rfid, sizeof exterior_rfid, exterior_rfid_text);

    access_HashResult hash = {};
    access_hash(hash, (char*)exterior_rfid, exterior_pin);
    access_user_IDX USER_idx = access_user_lookup(hash);

    if (add_next_user != add_user_state_NOT_ADDING) {
        // TODO enforce pin complexity?

        u8 extending = add_next_user == add_user_state_EXTENDING;
        char const * msg = access_user_add(hash, access_now_day() + 30, extending, 0);
        if (msg) {
            exterior_display("Cannot add\n%s", msg);
        } else {
            exterior_display("User Added\ndays_left:30");
            save_config();
        }

        add_next_user = add_user_state_NOT_ADDING;

    } else if (strcmp(exterior_option, "") == 0) { // Accesss request

        if (USER_idx == access_user_NOT_FOUND) {
            exterior_display("ACCESS DENIED\nUnknown User");
        } else {
            u16 expire_day = access_users_space[USER_idx].expire_day;
            if (expire_day == access_expire_day_magics_NewAdder ||
                expire_day == access_expire_day_magics_Adder ||
                expire_day == access_expire_day_magics_NewExtender ||
                expire_day == access_expire_day_magics_Extender) {
                    exterior_display("ACCESS GRANTED\n");
                    unlock_door();
            } else {

                s32 days_left = access_user_days_left(USER_idx);
                u32 secs_till_open_ = secs_til_open(day_sec_open, day_sec_close, now_ms());

                if (!secs_till_open_) {
                    if (days_left<0) {
                        exterior_display("ACCESS DENIED\nExpired");
                    } else {
                        exterior_display("ACCESS GRANTED\ndays_left:%d", days_left);
                        unlock_door();
                    }
                } else {
                    s32 sto_hr = (secs_till_open_ / 60) / 60;
                    s32 sto_mn = (secs_till_open_ / 60) % 60;
                    exterior_display("Sorry closed\n Wait %02d:%02d", sto_hr, sto_mn);
                }
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
                exterior_display("Request sending %s\nDay:%hu Idx:%hu", cancel_text, day, idx);

                INFO_HEXBUFFER(hash, sizeof hash, "Requested send   Day:%hu Idx:%hu :", day, idx);
                {
                    u8* h = hash;
                    r = snprintf(emailed_hash_buf, sizeof emailed_hash_buf,
                        "Day: %hu Idx: %hu\n"
                        "Hash: "
                        "%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x",
                        day, idx,
                        h[ 0], h[ 1], h[ 2], h[ 3], h[ 4], h[ 5], h[ 6], h[ 7],
                        h[ 8], h[ 9], h[10], h[11], h[12], h[13], h[14], h[15],
                        h[16], h[17], h[18], h[19], h[20], h[21], h[22], h[23],
                        h[24], h[25], h[26], h[27], h[28], h[29], h[30], h[31]
                        );
                }
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
                exterior_display("\nCant send, email busy, try again to cancel and send again");
                IO_TIMER_MS(clear_display) = now_ms() + 2000;
                state = STATE_SENDING_CANCELLED;
            } break;
        }
    } else if (strcmp(exterior_option, "200") == 0) {
        // add new user or extend existing one

        if (USER_idx == access_user_NOT_FOUND) {
            exterior_display("DENIED:\nUnknown User");
        } else {
            u16 expire_day = access_users_space[USER_idx].expire_day;
            if (expire_day == access_expire_day_magics_NewAdder ||
                expire_day == access_expire_day_magics_Adder) {

                add_next_user = add_user_state_ADDING_NEW;
                // TODO add timer to reset this
                exterior_display("\nWill add next user");

            } else if (expire_day == access_expire_day_magics_NewExtender ||
                       expire_day == access_expire_day_magics_Extender) {

                add_next_user = add_user_state_EXTENDING;
                // TODO add timer to reset this
                exterior_display("\nWill extend next user");
            } else {
                exterior_display("DENIED: you don't\nhave permission");
            }
        }
    } else if (strcmp(exterior_option, "301") == 0) {
        // Force config refresh

        if (USER_idx == access_user_NOT_FOUND) {
            exterior_display("DENIED:\nUnknown User");
        } else {
            u16 expire_day = access_users_space[USER_idx].expire_day;
            if (expire_day == access_expire_day_magics_NewAdder ||
                expire_day == access_expire_day_magics_Adder   ||
                expire_day == access_expire_day_magics_NewExtender ||
                expire_day == access_expire_day_magics_Extender) {
                exterior_display("Forcing config\nrefresh");
                INFO("Forcing config refresh");
                config_download_timeout();
            } else {
                exterior_display("DENIED: you don't\nhave permission");
            }
        }
    } else if (strcmp(exterior_option, "302") == 0) {
        struct tm _tm;
        time_t now_sec = now_ms() / 1000;
        localtime_r(&now_sec, &_tm);
        exterior_display("Current Time\n %02d:%02d ", _tm.tm_hour, _tm.tm_min);
    } else {
        exterior_display("\nUnkown option");
        ERROR("Got an unknown option from the exterior: %s", exterior_option);
    }
}

static void exterior_restart() {
    exterior_display("Exterior restart\nDetected");
    WARN("Exterior restart detected");
}


#define exterior_set(dest)  __exterior_set(start, end, dest, #dest, sizeof dest); return
#include "/build/exterior_protocol.re.c"


void emailed_hash_io_curl_complete(CURL *easy, CURLcode result, struct emailed_hash_CurlCtx * ctx) {
    email_free(&emailed_hash_email_ctx);
    state = STATE_IDLE;
}


u64 config_download_interval_sec = 60 * 60; // 1 Hour
//char * config_download_url = "http://127.0.0.1:9160/workspaces/the-bike-shed/shed_test_config";
//char * config_download_url = "http://192.168.4.159:9160/workspaces/the-bike-shed/shed_test_config";
char * config_download_url;

char config_download_previous_etag[32];
u64 config_download_previous_modified_time_sec;
static struct config_download_Ctx config_download_ctx;

static u64 last_config_download_sec = 0;


u8 admin_added;
void config_download_finished(struct config_download_Ctx *c, u8 success) {
    LOGCTX(" download_finished");
    memset(config_download_previous_etag, 0, sizeof config_download_previous_etag);
    strncpy(config_download_previous_etag, c->etag, sizeof config_download_previous_etag);
    IO_TIMER_MS(config_download) = (last_config_download_sec + config_download_interval_sec) * 1000;

    DEBUG("success:%d admin_added:%d", success, admin_added);

    if (config_memory_dirty) {
        config_memory_copy(&email_from);
        config_memory_copy(&email_host);
        config_memory_copy(&email_user_pass);
        config_memory_copy(&email_user_pass);
        config_memory_copy(&email_rcpt);
        config_memory_copy(&config_download_url);
        config_memory_copy(&serial_path);
    }
    config_memory_dirty = 2;

    if (success) {
        if (admin_added) {
            admin_added = 0;
            access_prune_not_new();
        }
        save_config();
    }

}

void config_download_timeout() {
  //DEBUG();
  config_download_abort(&config_download_ctx);
  last_config_download_sec = now_sec();
  memset(&config_download_ctx.line_accumulator_data, 0, sizeof config_download_ctx.line_accumulator_data);
  config_memory_dirty = 0;
  __config_download_start(
    &config_download_ctx,
    config_download_url,
    config_download_previous_etag,
    config_download_previous_modified_time_sec);

  IO_TIMER_MS(config_download) = (last_config_download_sec + config_download_interval_sec) * 1000;
}

void __debug_config_download_complete_hook() {};
struct StringList tmp_arg;


char * email_from;
char * email_host;
char * email_user_pass;
char * email_rcpt;
//char * serial_path = "/build/exterior_mock.pts";
//char * serial_path = "/dev/ttyAMA0";
char * serial_path = serial_path_DEFAULT;


void config_user_adder(char* hex) {
    admin_added = 1;
    access_HashResult hash = {};
    buf_from_hex(hash, sizeof hash, hex);
    access_user_add(hash, access_expire_day_magics_NewAdder, 0, 1);
}

void config_user_extender(char* hex) {
    admin_added = 1;
    access_HashResult hash = {};
    buf_from_hex(hash, sizeof hash, hex);
    access_user_add(hash, access_expire_day_magics_NewExtender, 0, 1);
}

void config_user_normal(char* hex, u16 expire_day) {
    access_HashResult hash = {};
    buf_from_hex(hash, sizeof hash, hex);
    access_user_add(hash, expire_day, 0, 1);
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

    u64 offset_ms = (7+4) * ms_per_hour; // UTC to PT + 4 hours;
    u64 utc_ms = now_ms();
    u16 next_maint_day = ( (utc_ms - offset_ms + ms_per_day) / ms_per_day ) ;
    u16 maint_day = next_maint_day - 1;
    u64 next_maint_ms = next_maint_day * ms_per_day  + offset_ms;

    DEBUG("%"PRIu64" %hu", next_maint_ms, maint_day);

    if (access_idle_maintenance_prev && *access_idle_maintenance_prev != (u16)-1) {
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
int main (int argc, char ** argv) {

    setlinebuf(stderr);

#if BUILD_IS_RELEASE == 0
    if (getenv("SHED_TRACE")) {
        INFO("Tracing enabled");
        log_trace_enabled = 1;
    }
#endif

    int r = putenv("TZ=:America/Los_Angeles");
    error_check(r);
    tzset();


    // This is an attempt to prevent memory allocations from being overcommitted,
    //  but I think this doesn't actually do that, it just prevents swaping out
    //  and we don't use a swap file anyway.... TODO: Research a better way to do this
    r = mlockall( MCL_CURRENT | MCL_FUTURE );
    error_check(r);

    assert(argc == 2);
    config_path = argv[1];

    access_user_list_init();

    config_initialize();

    log_allowed_fails = 100000000;
    config_backup_path_SET;
    if (!access(config_backup_path, F_OK)) {
        ERROR("Backup config file present, reading it instead main config");
        config_load_file(config_backup_path);
        save_config();
    } else {
        config_load_file(config_path);
    }
    config_memory_dirty = 2;

    access_prune_not_new(); // convert all the added into not added
    admin_added = 0;

    io_initialize();
    io_curl_initialize();
    serial_io_initialize(serial_path);
    gpio_pwm_initialize();




    assert(email_from);
    assert(email_host);
    assert(email_user_pass);
    assert(email_rcpt);

    // Wait 5 seconds to start downloading config,
    //  internet might not be available right away
    IO_TIMER_MS(config_download) = now_ms() + config_download_startup_delay_ms;

    idle_timeout();

    exterior_display("System restart\n");

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