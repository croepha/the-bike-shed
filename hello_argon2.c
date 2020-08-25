
#include "argon2.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define HASHLEN 32
#define PWD "password"


// The point of all this is to try to make argon2 not use any dynamic memory
static const uint32_t m_cost = (1<<17);
uint8_t static_memory_buf[m_cost << 10];
uint8_t static_memory_buf_in_use_debug;

static int allocate_memory(uint8_t **memory, size_t bytes_to_allocate) {
    if (bytes_to_allocate == sizeof static_memory_buf && !static_memory_buf_in_use_debug) {
        static_memory_buf_in_use_debug = 1;
        *memory = static_memory_buf;
    } else {
        printf("ERROR: Could not use static memory, falling back to malloc bytes:%zu in_use:%c",
            bytes_to_allocate, static_memory_buf_in_use_debug
        );
        assert(0);
        *memory = malloc(bytes_to_allocate);
    }
    printf("allocate_memory: %p bytes:%zu\n", *memory, bytes_to_allocate);
    return 0;
}
static void deallocate_memory(uint8_t *memory, size_t bytes_to_allocate) {
    if (memory == static_memory_buf) {
        static_memory_buf_in_use_debug = 0;
    } else {
        free(memory);
    }
    printf("deallocate_memory: %p bytes:%zu\n", memory, bytes_to_allocate);
}

char system_secret[32];

int main2(void)
{
    uint8_t hash2[HASHLEN];

    uint8_t* salt = (uint8_t*)"somesalt";
    int SALTLEN = strlen((char*)salt);

    uint8_t *pwd = (uint8_t *)strdup(PWD);
    uint32_t pwdlen = strlen((char *)pwd);

    uint32_t t_cost = 4;
    uint32_t parallelism = 1;

    // low-level API
    argon2_context context = {
        hash2,  /* output array, at least HASHLEN in size */
        HASHLEN, /* digest length */
        pwd, /* password array */
        pwdlen, /* password length */
        salt,  /* salt array */
        SALTLEN, /* salt length */
        NULL, 0, /* optional secret data */
        NULL, 0, /* optional associated data */
        t_cost, m_cost, parallelism, parallelism,
        ARGON2_VERSION_13, /* algorithm version */
        allocate_memory, deallocate_memory, /* custom memory allocation / deallocation functions */
        /* by default only internal memory is cleared (pwd is not wiped) */
        ARGON2_DEFAULT_FLAGS
    };

    int rc = argon2d_ctx( &context );
    if(ARGON2_OK != rc) {
        printf("Error: %s\n", argon2_error_message(rc));
        exit(1);
    }
    free(pwd);

    for( int i=0; i<HASHLEN; ++i ) printf( "%02x", hash2[i] ); printf( "\n" );
    return 0;
}


int main () {
    setbuf(stdout, 0);
#if 1
    return main2();
#else
    run(32, "password", 8, "somesalt", 4, 1<<17, 1, 1, Argon2_d, 0,0, ARGON2_VERSION_13);

#endif
}