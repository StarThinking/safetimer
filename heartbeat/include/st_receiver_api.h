#ifndef ST_RECEIVER_API
#define ST_RECEIVER_API

int safetimer_check(long app_id, char *node_ip, long start);
void receive_heartbeat(long app_id, char *node_ip, long state);
int init_st_receiver_api();
void destroy_st_receiver_api();

#endif
