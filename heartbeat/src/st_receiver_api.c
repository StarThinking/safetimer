#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "hashtable.h"

#include "hb_common.h"
#include "helper.h"
#include "hb_server.h"
#include "queue.h"
#include "barrier.h"

static hash_table app_nodes_ht;

static void cleanup_ht();

// return 0, if failure is not sure;
// return 1, if it is sure that failure occurs.
int safetimer_check(long app_id, char *node_ip, long start) {
        size_t value_size;
        hash_table * node_states_ht_p;
        hash_table ** node_states_ht_pp;
        long * node_state;
        long st_timestamp;

        node_states_ht_pp = (hash_table **) ht_get(&app_nodes_ht, &app_id, sizeof(long), &value_size);

	if (node_states_ht_pp == NULL) {
                printf("error: get_state can't find app_id %ld\n", app_id);
                return 0;
	} else {
                node_states_ht_p = *node_states_ht_pp;
                node_state = (long *) ht_get(node_states_ht_p, node_ip, strlen(node_ip), &value_size);
                if (node_state == NULL) {
                        printf("error: get_state can't find entry for app_id %ld node_ip %s\n", 
                                app_id, node_ip);
                        return 0;
                } else {
                        printf("\nThe state of node %s for app %ld is %ld.\n\n",
                                    node_ip, app_id, *node_state);
                        st_timestamp = *node_state;
                }
        }

        if (st_timestamp < start)
                return 1;

        return 0;
}

void receive_heartbeat(long app_id, char *node_ip, long state) { // state is timestamp
        size_t value_size;
        hash_table * node_states_ht_p;
        hash_table **node_states_ht_pp; // pointer of pointer
        long *node_state_p;
        //long *tmp;

        node_states_ht_pp = (hash_table **) ht_get(&app_nodes_ht, &app_id, sizeof(long), &value_size);
        if (node_states_ht_pp == NULL) {
                node_states_ht_p = (hash_table *) calloc(sizeof(hash_table), 1);
                ht_init(node_states_ht_p, HT_NONE, 0.05);
                node_states_ht_pp = &node_states_ht_p;
                ht_insert(&app_nodes_ht, &app_id, sizeof(long), node_states_ht_pp, sizeof(hash_table **));
                printf("inserted new hashtable for app %ld.\n", app_id);
        } else {
                node_states_ht_p = *node_states_ht_pp;
        }
        
        //printf("sizeof(*node_ip) = %lu\n", sizeof(*node_ip));

        node_state_p = (long *) ht_get(node_states_ht_p, node_ip, strlen(node_ip), &value_size);
        if (node_state_p == NULL) {
                long _node_state = 0;
                ht_insert(node_states_ht_p, node_ip, strlen(node_ip), &_node_state, sizeof(long));
                printf("inserted new entry for node %s\n", node_ip);
        } else {
                if (*node_state_p != state) {
                        printf("change the state of app %ld, node %s to %ld\n", 
                                app_id, node_ip, state);
                        *node_state_p = state;
                }
        }

        return;
}

int init_st_receiver_api() {
         int ret = 0;

         ht_init(&app_nodes_ht, HT_NONE, 0.05);
         
         if ((ret = init_queue()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init queue.\n");
                goto error;
         }

#ifdef CONFIG_BARRIER         
         if ((ret = init_barrier()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init barrier.\n");
                goto error;
         }
#endif

         if ((ret = init_hb()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init heartbeat server.\n");
                goto error;
         }

         printf("init_st_receiver_api() done.\n");

error:
         return ret;
}

void destroy_st_receiver_api() {
	cleanup_ht();

#ifdef CONFIG_BARRIER
        cancel_barrier();
        join_barrier();
#endif
        cancel_hb();
        join_hb();

        cancel_queue();
        join_queue();

        printf("destroy_st_receiver_api() done.\n");
}

static void cleanup_ht() {  
        unsigned int key_count;
        int i;
        void **keys;
        hash_table ** node_states_ht_pp;
        size_t value_size;
        
        printf("clean up hash table.\n");

        keys = ht_keys(&app_nodes_ht, &key_count);
        if (keys != NULL) {
                for (i=0; i<key_count; i++) {
                        long * key = keys[i];
                        printf("key = %ld\n", *key);
                        node_states_ht_pp = (hash_table **) ht_get(&app_nodes_ht, key, sizeof(long), &value_size);
                        if (node_states_ht_pp != NULL && *node_states_ht_pp != NULL) {
                                ht_destroy(*node_states_ht_pp);
                                free(*node_states_ht_pp);
                                printf("ht_destroy node_states_ht for app %ld\n", *key);
                        }
                }
                free(keys);
        }
        ht_destroy(&app_nodes_ht);
}
