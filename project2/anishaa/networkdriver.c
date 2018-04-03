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

// Size of bounded buffer for both send and receive side
#define SIZE (10)

// For debug purposes, cont the number of packets successfully sent and received
int send_packet_count = 0;
int recv_packet_count = 0;

typedef int Boolean;
#define False   (0)
#define True    (!False)

// Define a static structure for send side and receive side bounded buffers
struct bounded_buffer {
    pthread_mutex_t lock;     /* protects buffer from multiple writers */
    pthread_cond_t non_empty;  /* for consumer to await more stuff */
    pthread_cond_t non_full;   /* for producer to await space for stuff */

    int maxlength;    /* size of buffer */
    int readP;        /* slot before one to read from (if data exists) */
    int writeP;      /* slot before one to write to (if space exists) */
    Boolean emptyF;  /* flag true => buffer is empty */

    void   *buffer[SIZE];   /* storage for the actual stuff held */
};

typedef struct bounded_buffer BoundedBuffer;
NetworkDevice   *myND;
FreePacketDescriptorStore *myFPDS;
BoundedBuffer   sendBB;
BoundedBuffer   recvBB[MAX_PID + 1];
Boolean         NdInitSucessful = False;

// Initialize a given BB
void nd_initBB(BoundedBuffer *newBB, int size) {

    newBB->maxlength = size;
    newBB->writeP = newBB->readP = size-1;
    newBB->emptyF = True;

    pthread_mutex_init(&newBB->lock, NULL);
    pthread_cond_init(&newBB->non_full, NULL);
    pthread_cond_init(&newBB->non_empty, NULL);

}

// this is a thread which waits on Send side BB for PDs to become available
// Once a PD is available, this function send it to network device
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

// this is a helper thread
// as soon as a packet arrives on network device, this thread is created to take care of 
// writing the PD to receive BB for the applicable pid
void *recvBB_writer_helper(void *args) {

    PacketDescriptor *pd = (PacketDescriptor *)args;

    PID pid = packet_descriptor_get_pid(pd);
    BoundedBuffer *bb = &recvBB[pid];
    if ((bb->writeP==bb->readP) && (!bb->emptyF)){
      blocking_put_pd(myFPDS, pd);      
      return NULL;
    }
    pthread_mutex_lock(&bb->lock);

    /* await space to store the value in */
    if ((bb->writeP==bb->readP) && (!bb->emptyF)){
      pthread_mutex_unlock(&bb->lock);
      return NULL;
    }

    /* advance the write index, cycling if necessary, and store value */
    bb->buffer[bb->writeP = (bb->writeP + 1) % bb->maxlength] = pd;

    /* the buffer definitely isn't empty at this moment */
    bb->emptyF = False;
    pthread_cond_signal(&bb->non_empty);

    pthread_mutex_unlock(&bb->lock);
    return NULL;
}

// this thread's job is to always keep a PD queued up for the network device
// when a packet arrives, it creates a helper thread to process it, while this 
// thread turns around to wait for another packet from network device
void *recvBB_writer(void *args) {
    BoundedBuffer *bb = (BoundedBuffer *)args;
	if (bb == NULL) {
		fprintf(stderr, "Bad args");
		return NULL;
	}


    for (;;) {
        PacketDescriptor *pd;

        blocking_get_pd(myFPDS, &pd);
        register_receiving_packetdescriptor(myND, pd);
        await_incoming_packet(myND);

        pthread_t tid;
        if (pthread_create(&tid, NULL, recvBB_writer_helper, pd)) {
            fprintf(stderr, "Unable to create recvBB_writer_helper\n");
            blocking_put_pd(myFPDS, pd);        
            continue;
        }
    }
}

/* any global variables required for use by your threads and your driver routines */
/* definition[s] of function[s] required for your thread[s] */
void init_network_driver(NetworkDevice *nd, void *mem_start,
    unsigned long mem_length, FreePacketDescriptorStore **fpds) {
    
    if ((nd == NULL) || (mem_start == NULL) || (mem_length <=0) || (fpds == NULL)) {
        fprintf(stderr, "Bad arguments to init_network_driver\n");
        return;
    }

    // Remember the network device
    myND = nd;

    /* create Free Packet Descriptor Store */
    *fpds = create_fpds();
    if (*fpds == NULL) {
        fprintf(stderr, "create_fpds returned NULL\n");
        return;
    }
    myFPDS = *fpds;

    /* load FPDS with packet descriptors constructed from mem_start/mem_length */
    int num_descriptors = create_free_packet_descriptors(*fpds, mem_start, mem_length);
    if (num_descriptors == 0) {
        fprintf(stderr, "create_free_packet_descriptors returned 0\n");
        return;
    }

    /* create any buffers required by your thread[s] */
    nd_initBB(&sendBB, SIZE);

    int i;
    for (i = 0; i <= MAX_PID; ++i) {
        nd_initBB(&recvBB[i], SIZE);
    }

    /* create any threads you require for your implementation */
    // Create a thread to read PDs from Send BB and submit the PD to Network driver
    pthread_t readSendTID;
    if (pthread_create(&readSendTID, NULL, sendBB_reader, &sendBB)) {
        fprintf(stderr, "Unable to create sendBB_reader\n");
        return;
    }

    // Create a thread to await packets from network driver, and write to Receive BB
    // for the given PID
    pthread_t writeRecvTID;
    if (pthread_create(&writeRecvTID, NULL, recvBB_writer, recvBB)) {
        fprintf(stderr, "Unable to create recvBB_writer\n");
        return;
    }

    NdInitSucessful = True;
    return;
}

// This function is called by fake applications to send a packet in 
// blocking manner - it blocks if the send BB is full 
void blocking_send_packet(PacketDescriptor *pd) {

    if (NdInitSucessful != True) {
        fprintf(stderr, "Network Driver not initialized\n");
        return;
    }

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

// This function is called by fake applications to send a packet in 
// non-blocking manner - if no space available in send BB,it just returns
int nonblocking_send_packet(PacketDescriptor *pd) {

    if (NdInitSucessful != True) {
        fprintf(stderr, "Network Driver not initialized\n");
        return 0;
    }

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

// this function is called by fake applications to get a packet
// in blocking manner - it blocks if recv BB is empty 
void blocking_get_packet(PacketDescriptor **pd, PID pid) {

    if (NdInitSucessful != True) {
        fprintf(stderr, "Network Driver not initialized\n");
        return;
    }

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


// this function is called by fake applications to get a packet
// in non-blocking manner - if receive BB is empty, it returns
int nonblocking_get_packet(PacketDescriptor **pd, PID pid) {

    if (NdInitSucessful != True) {
        fprintf(stderr, "Network Driver not initialized\n");
        return 0;
    }

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
