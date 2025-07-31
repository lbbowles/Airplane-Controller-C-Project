#include "alist.h"
#include "airplane.h"

typedef struct {
    alist list;
} queue;

void queue_init();

void queue_enqueue(void *item); 

void queue_destroy(queue *q); 

void req_pos_request(airplane *plane, const char *flight_id); 

void req_ahead_request(airplane *plane, const char *flight_id);  

void *queue_handler_thread(void *arg); 

void taxiqueue_inair(airplane *plane, const char *flight_id);

