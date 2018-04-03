/*
Author: Anisha Aggarwal
DuckID: anishaa
Title: CIS 415 Project 1
File: networkdriver.c
Statement: All of the work included in this file is my own.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "networkdriver.h"
#include "packetdescriptorcreator.h"
#include "freepacketdescriptorstore__full.h"
#include "pid.h"

// Block buffer size - 10 for send side, and 5 for receive side
#define SEND_BBSIZE (5)
#define RECV_BBSIZE (5)
int send_packet_count = 0;
int recv_packet_count = 0;

typedef int Boolean;
#define False   (0)
#define True    (!False)

struct bounded_buffer {
    pthread_mutex_t lock;      /* protects buffer from multiple writers */
    pthread_cond_t non_empty;  /* for consumer to await more stuff */
    pthread_cond_t non_full;   /* for producer to await space for stuff */

    int maxlength;      /* size of buffer */
    int readP;          /* slot before one to read from (if data exists) */
    int writeP;         /* slot before one to write to (if space exists) */
    Boolean emptyF;     /* flag true => buffer is empty */

    void   **buffer;    /* storage for the actual stuff held */
};

typedef struct bounded_buffer BoundedBuffer;
NetworkDevice *myND;
FreePacketDescriptorStore *myFPDS;
BoundedBuffer sendBB;
BoundedBuffer recvBB[MAX_PID + 1];

// This function creates the bounded buffer of given size
int nd_createBB(BoundedBuffer *newBB, int size, void **memory, unsigned long *length) {

    newBB->buffer = (void **)(*memory);
	*length -= size*sizeof(void *);
		printf("len %ld\n", *length);
	if (*length <= 0) {
		printf("bad len \n");
		return -1;
	}
    *memory += size*sizeof(void *);

    newBB->maxlength = size;
    newBB->writeP = newBB->readP = size-1;
    newBB->emptyF = True;

    pthread_mutex_init(&newBB->lock, NULL);
    pthread_cond_init(&newBB->non_full, NULL);
    pthread_cond_init(&newBB->non_empty, NULL);

    return 0;
}
/*
void nd_destroyBB(BoundedBuffer *bb) {
    printf("nd_destroyBB\n");
}
*/
void *sendBB_reader(void *args) {
    for(;;) {
        BoundedBuffer *bb = (BoundedBuffer *)args;
        PacketDescriptor *pd;

        pthread_mutex_lock(&bb->lock);

        /* await a value to hand to our caller */
        while (bb->emptyF)
            pthread_cond_wait(&bb->non_empty, &bb->lock);

        /* advance the read index, cycling if necessary, and retrieve value */
        void *value;
        value = bb->buffer[bb->readP = (bb->readP + 1) % bb->maxlength];
        pd = (PacketDescriptor *)value;

        /* the buffer definitely isn't full at this moment */
        bb->emptyF = (bb->readP == bb->writeP);
        pthread_cond_signal(&bb->non_full);

        pthread_mutex_unlock(&bb->lock); 

        if (send_packet(myND, pd) == 0) {
            fprintf(stderr, "send_packet failed\n");
        } else {
          send_packet_count++;
        }

        blocking_put_pd(myFPDS, pd);
    }
}

void *recvBB_writer(void *args) {
    for (;;) {
        PacketDescriptor *pd;

        // Get a PD, register with network device and wait for incoming packet
        blocking_get_pd(myFPDS, &pd);
        register_receiving_packetdescriptor(myND, pd);
        await_incoming_packet(myND);

        // If there is no space in the BB for this PID, drop the packet
        PID pid = packet_descriptor_get_pid(pd);
	BoundedBuffer *bb = &(((BoundedBuffer *)args)[pid]);
        if ((bb->writeP==bb->readP) && (!bb->emptyF)){
            blocking_put_pd(myFPDS, pd);        
            return NULL;
        }

        // There is potentially space in the BB for this PID
        pthread_mutex_lock(&bb->lock);

        // Check if there is still space in the BB for this PID
        if ((bb->writeP==bb->readP) && (!bb->emptyF)){
            pthread_mutex_unlock(&bb->lock);
            blocking_put_pd(myFPDS, pd);        
            return NULL;
        }

        // advance the write index, cycling if necessary, and store value
        bb->buffer[bb->writeP = (bb->writeP + 1) % bb->maxlength] = pd;

        // the buffer definitely isn't empty at this moment 
        bb->emptyF = False;
        pthread_cond_signal(&bb->non_empty);

        pthread_mutex_unlock(&bb->lock);
        return NULL;
    }
}
/* any global variables required for use by your threads and your driver routines */
/* definition[s] of function[s] required for your thread[s] */
void init_network_driver(NetworkDevice *nd, void *mem_start,
    unsigned long mem_length, FreePacketDescriptorStore **fpds) {
    
    myND = nd;

    /* create Free Packet Descriptor Store */
    *fpds = create_fpds();
    if (*fpds == NULL) {
        fprintf(stderr, "create_fpds returned NULL\n");
        return;
    }
    myFPDS = *fpds;

    //printf("Initial mem_start 0x%x\n", (unsigned int)mem_start);
    /* create any buffers required by your thread[s] */
    int val = nd_createBB(&sendBB, SEND_BBSIZE, &mem_start, &mem_length);
    if (val == -1) {
        fprintf(stderr, "createBB send failed\n");
        goto clean_up;
    }

    //printf("After SendBB: mem_start 0x%x\n", mem_start);

    // Create one BB for each PID
    int i;
    for (i = 0; i <= MAX_PID; ++i) {
        val = nd_createBB(&recvBB[i], RECV_BBSIZE, &mem_start, &mem_length);
        if (val == -1) {
            fprintf(stderr, "createBB recv failed\n");
            goto clean_up;
        }
        //printf("After RecvBB %d: mem_start 0x%x\n", i, mem_start);
    }

    /* load FPDS with packet descriptors constructed from mem_start/mem_length */
    int num_descriptors = create_free_packet_descriptors(*fpds, mem_start, mem_length);
	printf("num_descriptors %d\n", num_descriptors);
    if (num_descriptors == 0) {
        fprintf(stderr, "create_free_packet_descriptors returned 0\n");
        return;
    }

    /* create any threads you require for your implementation */
    pthread_t readSendTID;
    if (pthread_create(&readSendTID, NULL, sendBB_reader, &sendBB)) {
        fprintf(stderr, "Unable to create sendBB_reader\n");
        goto clean_up;
    }

    pthread_t writeRecvTID;
    if (pthread_create(&writeRecvTID, NULL, recvBB_writer, recvBB)) {
        fprintf(stderr, "Unable to create recvBB_writer\n");
        goto clean_up;
    }
    return;
    
    clean_up:
/*
        if (sendBB != NULL) {
            //nd_destroyBB(sendBB);
        }
        for (i = 0; i <= MAX_PID; ++i) {
            if (recvBB[i] != NULL) {
                nd_destroyBB(recvBB[i]);
            }
        }
*/        
        return;
}

void blocking_send_packet(PacketDescriptor *pd) {
    /* queue up packet descriptor for sending */
    /* do not return until it has been successfully queued */
    BoundedBuffer *bb = &sendBB;

    pthread_mutex_lock(&bb->lock);

    /* await space to store the value in */
    while ((bb->writeP==bb->readP) && (!bb->emptyF))
        pthread_cond_wait(&bb->non_full, &bb->lock);

    /* advance the write index, cycling if necessary, and store value */
    bb->buffer[bb->writeP = (bb->writeP + 1) % bb->maxlength] = pd;

    /* the buffer definitely isn't empty at this moment */
    bb->emptyF = False;
    pthread_cond_signal(&bb->non_empty);

    pthread_mutex_unlock(&bb->lock);
    
}

int nonblocking_send_packet(PacketDescriptor *pd) {
    BoundedBuffer *bb = &sendBB;

    /* if you are able to queue up packet descriptor immediately, do so and return 1 */
    /* otherwise, return 0 */
    if ((bb->writeP == bb->readP) && (!bb->emptyF)) {
        return 0;
    } 

    pthread_mutex_lock(&bb->lock);

    if ((bb->writeP == bb->readP) && (!bb->emptyF)) {
        pthread_mutex_unlock(&bb->lock);
        return 0;
    }

    bb->buffer[bb->writeP = (bb->writeP + 1) % bb->maxlength] = pd;
    bb->emptyF = False;

    pthread_cond_signal(&bb->non_empty);

    pthread_mutex_unlock(&bb->lock);

    return 1;
    
}


void blocking_get_packet(PacketDescriptor **pd, PID pid) {
    BoundedBuffer *bb = &recvBB[pid];
    /* wait until there is a packet for `pid’ */
    pthread_mutex_lock(&bb->lock);

    /* await a value to hand to our caller */
    while (bb->emptyF)
        pthread_cond_wait(&bb->non_empty, &bb->lock);

    /* return that packet descriptor to the calling application */
    /* advance the read index, cycling if necessary, and retrieve value */
    void *value;
    value = bb->buffer[bb->readP = (bb->readP + 1) % bb->maxlength];
    *pd = (PacketDescriptor *)value;

    /* the buffer definitely isn't full at this moment */
    bb->emptyF = (bb->readP == bb->writeP);
    pthread_cond_signal(&bb->non_full);

    pthread_mutex_unlock(&bb->lock); 

    recv_packet_count++;
}


int nonblocking_get_packet(PacketDescriptor **pd, PID pid) {
    BoundedBuffer *bb = &recvBB[pid];
     /* if there is currently a waiting packet for `pid’, return that packet */
     /* to the calling application and return 1 for the value of the function */
     /* otherwise, return 0 for the value of the function */
    if (bb->emptyF) {
        return 0;
    }

    pthread_mutex_lock(&bb->lock);

    if (bb->emptyF) {
        pthread_mutex_unlock(&bb->lock);    
        return 0;
    }

    void *value;
    value = bb->buffer[bb->readP = (bb->readP + 1) % bb->maxlength];
    *pd = (PacketDescriptor *)value;

    /* the buffer definitely isn't full at this moment */
    bb->emptyF = (bb->readP == bb->writeP);
    pthread_cond_signal(&bb->non_full);

    pthread_mutex_unlock(&bb->lock);    

    recv_packet_count++;
    return 1;
}
