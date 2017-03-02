#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

struct qnode* new_node(long t, bool e) {
    struct qnode *temp = (struct qnode*)malloc(sizeof(struct qnode));
    temp->timestamp = t;
    temp->expired = e;
    temp->next = NULL;
    return temp; 
}
 
struct queue *create_queue() {
    struct queue *q = (struct queue*)malloc(sizeof(struct queue));
    q->front = q->rear = NULL;
    return q;
}

// find the qnode to which timestamp t belongs, and update expired to false
int update_queue(struct queue *q, long t) {
    printf("update_queue for t = %ld\n", t);
    if(q->front == NULL)
        return -1;
    
    struct qnode *p = q->front;
    // find the first expiration time that is larger than t
    while(p != NULL && (p->timestamp < t)) {
        p = p->next;
    }
    
    if(p != NULL) {
        printf("update heartbeat for expiration time %ld\n", p->timestamp);
        p->expired = false;
    } else {
        return -2;
    }

    printf("return\n");
    return 0;
}

void enqueue(struct queue *q, long t, bool e) {
    // Create a new LL node
    struct qnode *temp = new_node(t, e);
 
    // If queue is empty, then new node is front and rear both
    if(q->rear == NULL) {
       q->front = q->rear = temp;
       return;
    }
 
    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}
 
struct qnode *dequeue(struct queue *q) {
    // If queue is empty, return NULL.
    if(q->front == NULL)
       return NULL;
 
    // Store previous front and move front one node ahead
    struct qnode *temp = q->front;
    q->front = q->front->next;
 
    // If front becomes NULL, then change rear also as NULL
    if(q->front == NULL)
       q->rear = NULL;
    return temp;
}

bool empty(struct queue *q) {
    return (q->front == NULL);
}

/*
int main() {
    struct queue *q = createqueue();
    enqueue(q, 10, true);
    struct qnode *temp = dequeue(q);
    if(temp != NULL) {
        printf("time = %ld\n", temp->timestamp);
        free(temp);
    }
    if(empty(q))
        printf("empty\n");
    return 0;
}
*/
