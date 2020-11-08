#pragma once
#include "common.h"

void supr_exec_child(void);
void supr_email_add_data_start(char**buf_, usz*buf_space_left);
void supr_email_add_data_finish(usz new_space_used);
void supr_test_hook_pre_restart(void);
void supr_test_hook_pre_wait(void);
void supr_main(void);

// LOW_THRESHOLDS: We will start considering sending logs once we have accumulated this much bytes/time
extern u32 supr_email_low_threshold_bytes;
extern u32 supr_email_rapid_threshold_secs;
extern u32 supr_email_low_threshold_secs;
extern u32 supr_email_timeout_secs;
