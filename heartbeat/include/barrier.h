#ifndef BARRIER
#define BARRIER

/*
 * 1. Start barrier server and configure em1 as the interface for receiving.
 * 2. Set up rx queue flows from em2 to all the rx queues on em1
 *    and configure em2 as the interface for sending.
 */
int init_barrier() ;

/*
 * Clear up all the resources used for barrier client and server.
 */
void destroy_barrier();

/*
 * Send barrier message via all connected rx queue flows from em2 to em1.
 * Barrier message consists of data of two longs:
 * 1.Epoch id;
 * 2.Index (0...IRQ_NUM) 
 */
int send_barrier_message(long epoch_id);

#endif
