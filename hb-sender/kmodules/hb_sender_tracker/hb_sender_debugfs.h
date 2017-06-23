#define BUFFERSIZE 20

extern long hb_send_compl_time;
extern long base_time;
extern long timeout_interval; // unit of ms

void debugfs_init(void);

void debugfs_exit(void);
