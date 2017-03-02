#include <stdbool.h>

struct qnode {
    long timestamp;
    bool expired;
    struct qnode *next;
};

struct queue {
    struct qnode *front, *rear; };

struct queue *create_queue();
void enqueue(struct queue *q, long t, bool e);
struct qnode *dequeue(struct queue *q);
bool empty(struct queue *q);
int update_queue(struct queue *q, long t);
