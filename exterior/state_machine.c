#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "state_machine.h"

void print_main_menu(char output[][20]) {
    // TODO: check for bad output variable.
    sprintf(output[0], "* : DOOR ENTRY");
    sprintf(output[1], "#%d : ADD ADMIN", PHIL_OPT);
    sprintf(output[2], "#%d : 30 DAY", DAY_30_OPT);
    sprintf(output[3], "MAIN MENU");
}

void user_to_string(const enum USER user, char *output) {
    switch (user) {
        case GENERAL_DOOR: {sprintf(output,""); break;}
        case PHILANTHROPIST: {sprintf(output, "PHIL"); break;}
        case ADMIN: {sprintf(output, "ADMIN"); break;}
        case THIRTY_DAY: {sprintf(output ,"30 DAY"); break;}
        default: {sprintf(output, ""); break;}
     }
}

void state_to_string(const enum STATE state, char *output) {
   switch (state) {
        case MAIN_MENU: {sprintf(output,"Main Menu"); break;}
        case DOOR_PIN: {sprintf(output, "Entering Pin"); break;}
        case RFID_BADGE: {sprintf(output, "Badge RFID"); break;}
        case OPTIONS: {sprintf(output ,"Options"); break;}
        case ACCESS_PAGE: {sprintf(output ,"Access Page"); break;}
        default: {sprintf(output, ""); break;}
  }
}

void easter_eggs(const char *pin_so_far, char *output){
    if (strcmp(pin_so_far, "666") == 0) {
        sprintf(output, "HAIL SATAN");
    } else if (strcmp(pin_so_far, "1337") == 0) {
        sprintf(output, "HACK THE PLANET!");
    } else if (strcmp(pin_so_far, "69") == 0) {
        sprintf(output, "Nice.");
    } else {
        sprintf(output, "");
    }
}

void print_door_pin(const enum USER user, const char *pin_so_far, char output[][20]) {
    sprintf(output[0], "PIN:  %s", pin_so_far); 
    sprintf(output[1], "");
    easter_eggs(pin_so_far, output[2]);
    char user_title[CODE_LEN];
    memset(user_title, ' ', CODE_LEN);
    user_to_string(user, user_title);
    sprintf(output[3], "ENTER %s PIN", user_title);
}

void print_rpi_output(const enum USER user, char const rpi_output[2][CODE_LEN], char output[][CODE_LEN]) {
    memset(output[0], '\0', CODE_LEN);
    memset(output[1], '\0', CODE_LEN);
    sprintf(output[3], "%.20s", rpi_output[0]);
    sprintf(output[4], "%.20s", rpi_output[1]);
}

void print_rfid_sign(const enum USER user, const char *pin_so_far, char output[][CODE_LEN]) {
    sprintf(output[0], "PIN:  %s", pin_so_far); 
    sprintf(output[1], "");
    easter_eggs(pin_so_far, output[2]);
    sprintf(output[3], "BADGE RFID");
}

void print_option(const enum USER user, const char *pin_so_far, char output[][CODE_LEN]) {
    sprintf(output[0], "OPT:  %s", pin_so_far); 
    easter_eggs(pin_so_far, output[1]);
    // TODO : Reference a centralized place where the options are known.
    sprintf(output[2], "ADD ADMIN: %d", PHIL_OPT);
    sprintf(output[3], "ADD 30 DAY: %d", DAY_30_OPT);
}

void display(const enum STATE state, const enum USER user, const struct internal_state shed_state, char output[][CODE_LEN]) {
    switch (state) {
        case MAIN_MENU: {print_main_menu(output); break;}
        case DOOR_PIN: {print_door_pin(user, shed_state.pin_so_far, output); break;}
        case RFID_BADGE: {print_rfid_sign(user, shed_state.pin_so_far, output); break;}
        case OPTIONS: {print_option(user, shed_state.option_so_far, output); break;}
        case ACCESS_PAGE: {print_rpi_output(user, shed_state.rpi_output, output); break;}
        case ADDING_NEXT_USER: {print_rpi_output(user, shed_state.rpi_output, output); break;}
        default: {print_main_menu(output); break;}
    }
}

enum STATE determine_next_state(const enum STATE state, const enum USER user, const struct internal_state shed_state) {
    if (shed_state.reset) {
        return MAIN_MENU;
    }
    switch (state) {
        case MAIN_MENU: {
            if (shed_state.key == '*') {
                return DOOR_PIN;
            } else if (shed_state.key == '#') {
                return OPTIONS;
            } else {
                return MAIN_MENU;
            }
       }
       case DOOR_PIN: {
            if (strlen(shed_state.pin_so_far) >= 3) {
                return RFID_BADGE;
            } else if (shed_state.key == '#') {
                return OPTIONS;
            } else {
                return DOOR_PIN;
            }
       }
       case RFID_BADGE: {
            if (!shed_state.rfid_set) {
                return RFID_BADGE;
            }             
            // Specializations of the ACCESS_GRANTED will be user-dependent, like entering the door code.
            switch (user) {
                case GENERAL_DOOR: {return ACCESS_PAGE;}
                case PHILANTHROPIST: {return ACCESS_PAGE;}
                case ADMIN: {return ADDING_NEXT_USER;}
                case THIRTY_DAY: {return ACCESS_PAGE;}
            }
       }
       case ADDING_NEXT_USER: {
           return ADDING_NEXT_USER;
       }
       case OPTIONS: {
           if (shed_state.key == '*') {
               return DOOR_PIN;
           } else {
               return OPTIONS;
           }
       }
       case ACCESS_PAGE: {
           return ACCESS_PAGE;
       }
       default: {
           return state;
       }
    }
}

enum USER determine_user(const enum STATE state, const enum USER prev_user, const struct internal_state shed_state) {
    if (shed_state.reset) {
        return GENERAL_DOOR;
    }
    if (state == OPTIONS) {
        if (strcmp(shed_state.pin_so_far, xstr(PHIL_OPT)) == 0) {
            return PHILANTHROPIST;
        } else if (strcmp(shed_state.pin_so_far, xstr(DAY_30_OPT)) == 0) {
            return ADMIN;
        }
    } else if (state == RFID_BADGE && prev_user == ADMIN) {
        return THIRTY_DAY;
    }

    // Default : return GENERAL_DOOR.
    return GENERAL_DOOR;
}
