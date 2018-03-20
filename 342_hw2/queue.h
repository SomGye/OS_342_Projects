/****************************************************************************/
/* File: queue.h
 * Author: Maxwell Crawford
 * Date: 2-27-16
 */
/****************************************************************************/

/*
*	Generic Priority Queue
*	HEADER FILE
*/

#include <stdio.h>

extern int    ______trace_switch;

// typedef struct {
// 	int key;
// 	int value;
// } data_t;

typedef struct queueNode {
    struct queueNode *next, *prev;
    void *data;
} QueueNode;

typedef struct queueType {
    QueueNode *head;
    QueueNode *tail;
    QueueNode *current;
} Queue;


void initQueue(Queue *theQueue);
void enQueue(Queue *theQueue, void *data);
void enQueueSorted(Queue *theQueue, void *data, int (*compareTo)()); //USE THIS ONLY
void *frontValue(Queue *theQueue);
QueueNode *frontNode(Queue *theQueue);
void *backValue(Queue *theQueue);
QueueNode *backNode(Queue *theQueue);
void *deQueue(Queue *theQueue);
void removeNode(Queue *theQueue, QueueNode *p);
QueueNode *findNode(Queue *theQueue, void *data, int (*compareTo)());
void *findValue(Queue *theQueue, void *data, int (*compareTo)());
void purge(Queue *theQueue, void *data, int (*compareTo)());
void printQ(Queue *theQueue, char *label, char *(*toString)()) ;
void fprintQ(FILE *fd, Queue *theQueue, char *label, char *(*toString)());
void fprintQN(FILE *fd, Queue *theQueue, char *label, char *(*toString)());
void setCurrent(Queue *theQueue, QueueNode *c);
void advance(Queue *theQueue);
void retreat(Queue *theQueue);
int isPastEnd(Queue *theQueue);
QueueNode *getCurrentNode(Queue *theQueue);
void *getCurrentValue(Queue *theQueue); 
