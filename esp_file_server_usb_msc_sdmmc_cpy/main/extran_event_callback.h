#ifndef EXTRA_EVENT_CALLBACK_H
#define EXTRA_EVENT_CALLBACK_H
#include <stdint.h>
#define MAX_CALLBACK 20

// create additional event callback
typedef void(* event_callback_t)(void* event_data);

typedef struct {
    event_callback_t callback[MAX_CALLBACK];
    uint8_t callback_registered_count;
} extra_event_callback;


#endif /* end of include guard: EXTRA_EVENT_CALLBACK_H */

