#include <inttypes.h>
#include <stdint.h>

#define CODE_LEN 20
#define PHIL_OPT 100
#define DAY_30_OPT 200
#define xstr(num) str(num)
#define str(num) #num

// Core enum for the shed state machine.
enum STATE {
    MAIN_MENU,
    DOOR_PIN,
    RFID_BADGE,
    OPTIONS,
    ACCESS_PAGE,
    ADDING_NEXT_USER
};

// Who is accessing (extra data to help inform display output).
enum USER {
    GENERAL_DOOR,
    PHILANTHROPIST,
    ADMIN,
    THIRTY_DAY,
};

// Loosely defined POD for shed data relevant to computing state and output.
struct internal_state {
    char key;
    // Pin typed in by the user
    char pin_so_far[11];
    // Option typed in by the user.
    // (We need both of these because the communication protocol is a one-size-fits-all
    // and can't be bothered to be split #sassysunday).
    char option_so_far[4];
    // Result string from the Raspberry pi.
    char rpi_output[2][CODE_LEN];
    // Result, parsed into an easy-to-define string.
    uint8_t rfid_set;
    // 1 if reset should happen, 0 otherwise
    uint8_t reset;
};

/// TODO: Are these things needed really?
void user_to_string(const enum USER user, char *output);
void state_to_string(const enum STATE state, char *output);

// Conversion of rpi output to RFID accepted / denied.
enum RFID get_rfid(const char rpi_output[2][CODE_LEN]);

// Printing functions.
void print_main_menu(char output[][CODE_LEN]);
void easter_eggs(const char *pin_so_far, char *output);
void print_door_pin(const enum USER user, const char *pin_so_far, char output[][CODE_LEN]);
void print_rpi_output(const enum USER user, char const rpi_output[2][CODE_LEN], char output[][CODE_LEN]);
void print_rfid_sign(const enum USER user, const char *pin_so_far, char output[][CODE_LEN]);
void print_option(const enum USER user, const char *pin_so_far, char output[][CODE_LEN]);


// Sets the output string based on current state and shed data (actual LED printing will be handled by original code).
void display(const enum STATE state, const enum USER user, const struct internal_state shed_state, char output[][CODE_LEN]);
enum STATE determine_next_state(const enum STATE state, const enum USER user, const struct internal_state shed_state);
enum USER determine_user(const enum STATE state, const enum USER prev_user, const struct internal_state shed_state);