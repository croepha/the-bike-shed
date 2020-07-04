
#include "argon2.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define HASHLEN 32
#define PWD "password"


#define UNUSED_PARAMETER(x) (void)(x)
#define clear_internal_memory(a,b)

static void fatal(const char *error) {
    fprintf(stderr, "Error: %s\n", error);
    exit(1);
}

static void print_hex(uint8_t *bytes, size_t bytes_len) {
    size_t i;
    for (i = 0; i < bytes_len; ++i) {
        printf("%02x", bytes[i]);
    }
    printf("\n");
}

/*
Runs Argon2 with certain inputs and parameters, inputs not cleared. Prints the
Base64-encoded hash string
@out output array with at least 32 bytes allocated
@pwd NULL-terminated string, presumably from argv[]
@salt salt array
@t_cost number of iterations
@m_cost amount of requested memory in KB
@lanes amount of requested parallelism
@threads actual parallelism
@type Argon2 type we want to run
@encoded_only display only the encoded hash
@raw_only display only the hexadecimal of the hash
@version Argon2 version
*/
static void run(uint32_t outlen, char *pwd, size_t pwdlen, char *salt, uint32_t t_cost,
                uint32_t m_cost, uint32_t lanes, uint32_t threads,
                argon2_type type, int encoded_only, int raw_only, uint32_t version) {
    size_t saltlen, encodedlen;
    int result;
    unsigned char * out = NULL;
    char * encoded = NULL;


    if (!pwd) {
        fatal("password missing");
    }

    if (!salt) {
        clear_internal_memory(pwd, pwdlen);
        fatal("salt missing");
    }

    saltlen = strlen(salt);
    if(UINT32_MAX < saltlen) {
        fatal("salt is too long");
    }

    UNUSED_PARAMETER(lanes);

    out = malloc(outlen + 1);
    if (!out) {
        clear_internal_memory(pwd, pwdlen);
        fatal("could not allocate memory for output");
    }

    result = argon2_hash(t_cost, m_cost, threads, pwd, pwdlen, salt, saltlen,
                         out, outlen, 0, 0, type,
                         version);
    if (result != ARGON2_OK)
        fatal(argon2_error_message(result));

    if (encoded_only)
        puts(encoded);

    if (raw_only)
        print_hex(out, outlen);

    if (encoded_only || raw_only) {
        free(out);
        free(encoded);
        return;
    }

    printf("Hash:\t\t");
    print_hex(out, outlen);
    free(out);

    result = argon2_verify(encoded, pwd, pwdlen, type);
    if (result != ARGON2_OK)
        fatal(argon2_error_message(result));
    printf("Verification ok\n");
    free(encoded);
}


int main2(void)
{
    uint8_t hash1[HASHLEN];
    uint8_t hash2[HASHLEN];

    uint8_t* salt = (uint8_t*)"somesalt";
    int SALTLEN = strlen((char*)salt);

    uint8_t *pwd = (uint8_t *)strdup(PWD);
    uint32_t pwdlen = strlen((char *)pwd);

    uint32_t t_cost = 4;
    uint32_t m_cost = (1<<17);
    uint32_t parallelism = 1;

    // high-level API
    argon2d_hash_raw(t_cost, m_cost, parallelism, pwd, pwdlen, salt, SALTLEN, hash1, HASHLEN);

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
        NULL, NULL, /* custom memory allocation / deallocation functions */
        /* by default only internal memory is cleared (pwd is not wiped) */
        ARGON2_DEFAULT_FLAGS
    };

    int rc = argon2d_ctx( &context );
    if(ARGON2_OK != rc) {
        printf("Error: %s\n", argon2_error_message(rc));
        exit(1);
    }
    free(pwd);

    for( int i=0; i<HASHLEN; ++i ) printf( "%02x", hash1[i] ); printf( "\n" );
    if (memcmp(hash1, hash2, HASHLEN)) {
        for( int i=0; i<HASHLEN; ++i ) {
            printf( "%02x", hash2[i] );
        }
        printf("\nfail\n");
    }
    else printf("ok\n");
    return 0;
}


int main () {
#if 1
    return main2();
#else
    run(32, "password", 8, "somesalt", 4, 1<<17, 1, 1, Argon2_d, 0,0, ARGON2_VERSION_13);

#endif
}