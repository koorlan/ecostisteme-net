#ifndef _NET_H
#define _NET_H 

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>


#define QUEUE_ELEMENTS 1000
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)

typedef struct QUEUE
{
	packetdata_t packet_info[QUEUE_SIZE];
	int queueIn,
		queueOut,
		size;
}queue_t;


void QueueInit(queue_t *queue)
{

    queue->queueIn = queue->queueOut = queue->size =0;
}

int QueuePut(queue_t *queue,packetdata_t packet_info)
{
    if(queue->queueIn == (( queue->queueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE))
    {
        return -1; /* Queue Full*/
    }

    queue->packet_info[queue->queueIn] = packet_info;

    queue->queueIn = (queue->queueIn + 1) % QUEUE_SIZE;

    queue->size ++;
    return 0; // No errors
}

int QueueGet(queue_t *queue,packetdata_t *packet_info)
{
    if(queue->queueIn == queue->queueOut)
    {
        return -1; /* Queue Empty - nothing to get*/
    }

    *packet_info = queue->packet_info[queue->queueOut];

    queue->queueOut = (queue->queueOut + 1) % QUEUE_SIZE;

    queue->size--;
    return 0; // No errors
}


#endif