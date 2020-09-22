
#define LOG_DEBUG

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include "logging.h"
#include "argon2.h"

#define HASHLEN 32


// The point of all this is to try to make argon2 not use any dynamic memory
static const uint32_t m_cost = (1<<17);
uint8_t static_memory_buf[m_cost << 10];
uint8_t static_memory_buf_in_use_debug;

static int allocate_memory(uint8_t **memory, size_t bytes_to_allocate) {
    if (bytes_to_allocate == sizeof static_memory_buf && !static_memory_buf_in_use_debug) {
        static_memory_buf_in_use_debug = 1;
        *memory = static_memory_buf;
    } else {
        ERROR("Could not use static memory, falling back to malloc bytes:%zu in_use:%c",
            bytes_to_allocate, static_memory_buf_in_use_debug
        );
        assert(0);
        *memory = malloc(bytes_to_allocate);
    }
    DEBUG("allocate_memory: %p bytes:%zu", *memory, bytes_to_allocate);
    return 0;
}
static void deallocate_memory(uint8_t *memory, size_t bytes_to_allocate) {
    if (memory == static_memory_buf) {
        static_memory_buf_in_use_debug = 0;
    } else {
        free(memory);
    }
    DEBUG("deallocate_memory: %p bytes:%zu", memory, bytes_to_allocate);
}

char system_secret[32];

struct access_HashPayload {
  u8 salt[64];
  u8 rfid[24];
  u8 pin[10];
};
typedef u8 access_HashResult[64];


#define PWD "password"

static void access_hash(access_HashResult * result, struct access_HashPayload * payload) {
    u32 const t_cost = 1;
    u32 const parallelism = 1;

    argon2_context context = {     // low-level API
        *result,  /* output array, at least HASHLEN in size */
        sizeof *result, /* digest length */
        (u8*)payload, /* password array */
        sizeof payload, /* password length */
        payload->salt,  /* salt array */
        sizeof payload->salt, /* salt length */
        NULL, 0, /* optional secret data */
        NULL, 0, /* optional associated data */
        t_cost, m_cost, parallelism, parallelism,
        ARGON2_VERSION_13, /* algorithm version */
        allocate_memory, deallocate_memory, /* custom memory allocation / deallocation functions */
        /* by default only internal memory is cleared (pwd is not wiped) */
        ARGON2_DEFAULT_FLAGS
    };
    int rc = argon2d_ctx( &context );
    if (rc != ARGON2_OK) {
        ERROR("argon2 error:%s", argon2_error_message(rc));
        memset(*result, 0, sizeof * result);
    }
}

static void access_hash_test(char * salt, char * rfid, char *  pin) {
    struct access_HashPayload payload = {};

#define _(v) \
    usz v ## _len = strlen(v); \
    assert(v ## _len <= sizeof payload. v); \
    memcpy(payload. v, v, v ## _len);

    _(salt);
    _(rfid);
    _(pin);
#undef _

    access_HashResult result = {};
    INFO_HEXBUFFER(&payload, sizeof payload, "payload:");
    access_hash(&result, &payload);
    INFO_HEXBUFFER(&result, sizeof result, "result:");
}

int main () {
    setbuf(stdout, 0);
    access_hash_test("000000000001", "0000000000001", "0000001");
    access_hash_test("000000000002", "0000000000001", "0000001");
    access_hash_test("000000000001", "0000000000002", "0000001");
    access_hash_test("000000000001", "0000000000001", "0000002");

}