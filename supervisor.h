
extern char * supr_email_rcpt;

void supr_exec_child();
void supr_email_add_data_start(char**buf_, usz*buf_space_left);
void supr_email_add_data_finish(usz new_space_used);
void supr_test_hook_pre_restart();
void supr_test_hook_pre_wait();
