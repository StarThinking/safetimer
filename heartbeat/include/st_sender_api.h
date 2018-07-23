#ifndef ST_SENDER_API
#define ST_SENDER_API

int safetimer_send_heartbeat(long app_id, long timeout_time, int node_id);
void safetimer_update_valid_time(long t);
int init_st_sender_api();
void destroy_st_sender_api();

#endif
