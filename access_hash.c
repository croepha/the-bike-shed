

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include "logging.h"
#include "argon2.h"
#include "access.h"


// The point of all this is to try to make argon2 not use any dynamic memory
static u32 const m_cost = (1<<16);
static u8 static_memory_buf[m_cost << 10];
static u8 static_memory_buf_in_use_debug;

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



void __access_hash(access_HashResult result, struct access_HashPayload * payload) {
    u32 const t_cost = 1;
    u32 const parallelism = 1;

    argon2_context context = {     // low-level API
        result,  /* output array, at least HASHLEN in size */
        sizeof(access_HashResult), /* digest length */
        (u8*)payload, /* password array */
        sizeof * payload, /* password length */
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
        memset(result, 0, sizeof(access_HashResult));
    }
}

