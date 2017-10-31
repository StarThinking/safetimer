#ifndef STATE_SERVER
#define STATE_SERVER

extern long node_state;

/*
 * Initialize state server.
 */
int init_state_server();

/*
 * Cancel.
 */
void cancel_state_server();

void join_state_server();

#endif
