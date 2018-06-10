#ifndef STATE_SERVER
#define STATE_SERVER

/*
 * Initialize state server.
 */
int init_state_server();

/*
 * Cancel.
 */
void cancel_state_server();

void join_state_server();

void receive_heartbeat(long app_id, char *node_ip, long state);
    
#endif
