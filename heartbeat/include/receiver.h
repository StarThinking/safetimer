#include "hb_config.h"

#ifndef RECEIVER
#define RECEIVER

//typedef void (*cb_t)();

int init_receiver(cb_t callback);

void destroy_receiver();

#endif
