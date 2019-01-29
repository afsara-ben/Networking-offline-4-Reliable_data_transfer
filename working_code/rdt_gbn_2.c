#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
 FROM VERSION 1.1 of J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg
{
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/


float TIMEOUT = 25.0; 
int WINSIZE;         
int SND_BUFSIZE = 50; //Sender's Buffer size
int RCV_BUFSIZE = 0; //Receiver's Buffer size

float time;

int base_A; //to maintain sender's lowest un-acknowledged packet. we start timer for this packet.
int nextseqnum_A; //sequence number to be given to next packet we will receive

int expected_seqNum_B; //receiver's expects this sequence number

struct pkt* packet_from_A; // to maintain a buffer at A of packets to be sent to B
struct pkt ack_from_B; // ack to be sent from B to A

int A_ackReceived_count;
int A_dropped_count;
int A_corruptReceived_count;
int A_timeoutOccurred_count;
int A_nackReceived_count;
int B_correctlyReceived_count;
int B_corruptReceived_count;
int B_duplicateReceived_count;

int calc_CheckSum (struct pkt packet)
{
  //printf(" in calc_CheckSum\n");
  //calculating checksum byte by byte
  int i;
  int checksum = 0;
  unsigned char byte;

  //printf("\n\n\tsizeof pkt seqnum : %ld, size of pkt acknum : %ld\n\n",sizeof(packet.seqnum), sizeof(packet.acknum) );
  for (i = 0; i < sizeof(packet.seqnum); i++)
  {
      byte = *((unsigned char *)&packet.seqnum + i);
      checksum += byte;
      //printf("byte is %d\n", byte );
      //printf("check is %d\n", checksum );
  }

  for (i = 0; i < sizeof(packet.acknum); i++)
  {
      byte = *((unsigned char *)&packet.acknum + i);
      checksum += byte;
      //printf("byte is %d\n", byte );
      //printf("check is %d\n", checksum );
  }

  for (i = 0; i < sizeof(packet.payload); i++)
  {
      byte = *((unsigned char *)&packet.payload + i);
      checksum += byte;
      //printf("byte is %d\n", byte );
      //printf("check is %d\n", checksum );
  }

  return checksum;
}


void printBuffer()
{
    printf("\n          **** Printing data in buffer **** \n");
    for (int i = 0; i < sizeof(packet_from_A); ++i)
    {
        printf("                %s\n", packet_from_A[i].payload );
    }
    printf("            **********************************\n\n");
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{

  printBuffer();

  /*printf("\n ...in A_output() ... \n\n");
  printf("[A_output] received data from layer5: %s \n", message.data);
  printf("[A_output] next seqnum is: %d\n", nextseqnum_A);
  printf("[A_output] Time: %f\n", time); */


 //make packet and always add packet to packet_from_A buffer
  memset(packet_from_A + nextseqnum_A, 0, sizeof(packet_from_A));
  
  packet_from_A[nextseqnum_A].seqnum = nextseqnum_A;
  
  memcpy(packet_from_A[nextseqnum_A].payload, message.data, 20);
  
  packet_from_A[nextseqnum_A].checksum = calc_CheckSum(packet_from_A[nextseqnum_A]);
  
  /*printf("[A_output] packet created with payload: %s \n", packet_from_A[nextseqnum_A].payload);
  printf("[A_output] packet added to buffer to resend if required\n");*/

  //send packet if it falls within window
  while ((nextseqnum_A < (base_A + WINSIZE )) && (packet_from_A[nextseqnum_A].seqnum != -1))
  {
     
    tolayer3(0, packet_from_A[nextseqnum_A]);  
    
    if( base_A == nextseqnum_A)
    {
      //restart timer
      printf("[A_output] Timer started for base %d\n", base_A);
      starttimer(0, TIMEOUT);
      
    }

    //printf("[A_output] Packet %d sent.\n", packet_from_A[nextseqnum_A].seqnum);
    printf("**** At %f in A_output() : Correct pkt received. pkt%d %s ****\n",time, packet_from_A[nextseqnum_A].seqnum, packet_from_A[nextseqnum_A].payload );
    nextseqnum_A++;    
 
  }
  
  //printf("out of [A_output]\n\n");  
  return;
}

/* need be completed only for extra credit */
void B_output(struct msg message)
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  
  int i;
/*  printf("\n ...in A_input() ... \n\n");
  printf("[A_input] Base: %d\n", base_A);
  printf("[A_input] nextseqnum_A: %d\n", nextseqnum_A);*/
  

  //printf("[A_input] ACK received: %d\n", packet.acknum);
  
  int checksum = calc_CheckSum(packet);
  printf("[A_input] Calculated Checksum : %d | Received checksum : %d\n", checksum, packet.checksum);

  
  //do nothing if checksum doesn't match. return
  if (checksum != packet.checksum)
  {
    printf("[A_input] Checksum does not match. Received ACK%d is corrupt\n",packet.acknum);
    return;
  }
  
  //packet is not corrupt
  //pkt er seqnum tai acknum diye pabo
  printf("**** At %f in A_input() : ACK%d received for pkt%d: %s ****\n",time,packet.acknum, packet.acknum,packet.payload );
  
  if (base_A != packet.acknum + 1)  //to avoid starting timer again if it has already been started
  { 
    //shift the base
    base_A = packet.acknum + 1;         //packet.acknum is received ack
    if(base_A == nextseqnum_A)          //stop timer when buffer is empty
        {
          stoptimer(0);
          printf("\n[A_input] Timer stopped for pkt%d.\n",packet.acknum);
        } 
    else
        {
          printf("Base : %d \nnextseqnum_A : %d\n", base_A, nextseqnum_A );      
          printf("\nbase_A is not equal to nextseqnum_A\n\n");
          if (packet.acknum != 0) 
            //nextseqnumn and acknum starts with 0.
            //to avoid starting timer again if 1st packet itself didnt get acked.
          {
            
            printf("\n[A_input] Timer stopped for pkt%d\n", packet.acknum);
            stoptimer(0);
            printf("[A_input] Timer started for base %d\n", base_A);
            starttimer(0, TIMEOUT);
          }
        }
  }

  printf("\n\n");      
  return;
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
  
  //printf("\n in A_timerinterrupt() \n\n");
  int i;
  printf("\n[A_timerinterrupt] Timer started for base %d\n", base_A); 
  

  //send all packets starting from base when timer goes off
  //start timer again     
  starttimer(0, TIMEOUT);
/*  printf("[A_timerinterrupt] nextseqnum_A: %d\n", nextseqnum_A);
  printf("[A_timerinterrupt] base_A: %d\n", base_A);
  printf("[A_timerinterrupt] Time: %f\n", time); */

  for (i = 0; i< (nextseqnum_A - base_A) ; i++)
  {
    printf("[A_timerinterrupt] Resending pkt%d %s \n",packet_from_A[base_A+i].seqnum ,packet_from_A[base_A+i].payload); 
    tolayer3(0, packet_from_A[base_A+i]);
  }

  printf("\n");
  return;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
  int i;
  packet_from_A = (struct pkt* )malloc( (sizeof(struct pkt))* SND_BUFSIZE);
  memset(packet_from_A, 0, (sizeof(struct pkt))* SND_BUFSIZE );

  for (i =0; i< SND_BUFSIZE; i++)
  {
    packet_from_A[i].seqnum = -1;
    
  }
  
  base_A = 0;
  nextseqnum_A =0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{

  //printf("\n ...in B_input() ...\n\n");
  /*printf("[B_input] Expected seqnum: %d\n", expected_seqNum_B); 
  printf("[B_input] Packet received: %s \n", packet.payload);
  printf("[B_input] Seqnum: %d\n", packet.seqnum);
  printf("[B_input] Time: %f\n", time); */
  

  int checksum = calc_CheckSum(packet);
  //printf("[B_input] Calculated Checksum: %d | Received checksum : %d\n", checksum, packet.checksum);

  // if pkt is not corrupt and is expected
  if (  (packet.checksum == checksum) && (packet.seqnum == expected_seqNum_B)  )
  {

    //printf("[B_input] Sequence number: %d \n", packet.seqnum);
    //printf("[B_input] Checksum matches.\n");

    tolayer5(1, packet.payload);
    
    //printf("[B_input] Data handed to layer5. ");
    printf("**** At %f in B_input() : Correct pkt received. pkt%d %s ****\n",time, packet.seqnum,packet.payload );
  
 
    memset(&ack_from_B, 0, sizeof(struct pkt));
    ack_from_B.acknum = expected_seqNum_B;
    ack_from_B.checksum = calc_CheckSum(ack_from_B);

    printf("[B_input] NOT Sending ACK%d to A\n", ack_from_B.acknum);
    //tolayer3(1, ack_from_B);  
    starttimer(1,TIMEOUT);
    expected_seqNum_B++;

    return;
  }

  //for corrupt pkt or mismatched seqnum
  if(packet.checksum != checksum)
  {
    printf("**** At %f in B_input() :  Checksum mismatch. Corrupt Packet received. Resending ACK%d for pkt%d %s ****\n",time, ack_from_B.acknum, packet.seqnum,packet.payload );  
  }
  else
  {
    printf("**** At %f in B_input() :  Duplicate Packet received. Resending ACK%d for pkt%d %s ****\n",time, ack_from_B.acknum, packet.seqnum,packet.payload );
  }  
  //tolayer3(1, ack_from_B);  
  starttimer(1,TIMEOUT);
 
  printf("\n");
  return;  
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
  
  int i;
  printf("\n[B_timerinterrupt] Timer started for ack %d\n", ack_from_B.acknum); 
  
  starttimer(1, TIMEOUT);


  for (i = 0; i< (base_A - ack_from_B.acknum) ; i++)
  {
    printf("[Btimerinterrupt] Resending ack%d for pkt%d %s\n",packet_from_A[base_A+i].acknum, packet_from_A[base_A+i].seqnum ,packet_from_A[base_A+i].payload); 
    tolayer3(1, ack_from_B);
  }

  printf("\n");
  return;
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
  expected_seqNum_B = 0;
  memset(&ack_from_B, 0, sizeof(struct pkt));
  ack_from_B.acknum = 0;
  ack_from_B.checksum = calc_CheckSum(ack_from_B);
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)

THERE IS NO REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOULD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER5)
        {
            //printf("\n\t\tsending %d th msg from layer 5\n\n",nsim+1 );
            
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 20; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[19] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER3)
        {
            //printf("\n\t\tsending %d th msg from layer 3\n\n",nsim );

            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give); /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt(); //always goes to A since unidirectional
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
        " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
        time, nsim);
}

void init() /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    WINSIZE = 7;

    nsimmax = 20; lossprob = 0.2; corruptprob =0.2; lambda=10; TRACE = 4;
    //nsimmax = 20; lossprob = 0.2; corruptprob =0.3; lambda=1000; TRACE = 4;
    printf("-----  Go Back-N Protocol -------- \n\n");
    printf("number of messages to simulate: %d\n",nsimmax);
    printf("packet loss probability : %f\n",lossprob);
    printf("packet corruption probability : %f\n",corruptprob);
    printf("average time between messages from sender's layer5 : %f\n",lambda);
    printf("TRACE: %d\n",TRACE);
    printf("Window size : %d\n",WINSIZE );

/*    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d",&nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f",&lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f",&corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf("%f",&lambda);
    printf("Enter TRACE:");
    scanf("%d",&TRACE);*/

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;                 /* individual students may need to change mmm */
    x = rand() / mmm;        /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        //printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
       // printf("            INSERTEVENT: time is %lf\n", time);
       // printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;      /* q points to header of list in which p struct inserted */
    if (q == NULL)   /* list is empty */
    {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)   /* end of list */
        {
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)     /* front of list */
        {
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else     /* middle of list */
        {
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
       // printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;          /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)   /* front of list - there must be event after */
            {
                q->next->prev = NULL;
                evlist = q->next;
            }
            else     /* middle of list */
            {
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to start timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        //printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet)
{
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet%d %s being lost\n",packet.seqnum, packet.payload);
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        /*printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");*/
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        //printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    int i;
    if (TRACE > 2)
    {
        /*printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");*/
    }
}

/*
 * algorithm to handle timer for all the packets using a single timer -
 * if the timer is not running then start timer and push the current time a queue.
 * if the timer is running then just push the current time in the queue
 * when we get an ACK we pop the first element
 * then if there is a packet in the window we start timer for it
 * but here the time_threshold for the pkt will be 'time_threshold - (current_time - time_when_it_was_pushed)'
 * now if the above formula gives negative ans then time has been passed for the pkt, so we give time_threshold = 0
 * in the timer_interrupt we start a timer for the first packet of the window, for the rest we save current time in the queue
 *
 * */