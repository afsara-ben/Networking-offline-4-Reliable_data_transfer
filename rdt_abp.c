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

float time = 0.000;
float TIMEOUT = 20.0;

struct pkt packet_from_A; //packet to be sent A to B
struct pkt ack_pkt_from_B; //ack to be sent from B to A

int A_SEQ; //state A
int B_ACK; //state B

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


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{

  /*printf("\n\n");
  printf("in A_output\n");  
  
  printf("(Sender) (in A_output) state is %d\n", A_SEQ);
  printf("(Sender) (in A_output) msg from upper layer is : %s \n", message.data);
  */
  //generate packet and start timer
  memset(&packet_from_A, 0, sizeof(packet_from_A));

  // if we are in state 0, seqnum is 0, else it is 1
  packet_from_A.seqnum  = (A_SEQ == 0)? 0 : 1;
  
  memcpy(&packet_from_A.payload, message.data, 20);

  packet_from_A.checksum = calc_CheckSum(packet_from_A);
  
  //printf("(Sender) (in A_output) Message in newly created packet : %s \n", packet_from_A.payload);
  
  tolayer3(0, packet_from_A); //tolayer3(from which entity, struct pkt)
  starttimer(0, TIMEOUT);

  printf("**** At %f in A_output() : Sending pkt%d %s ****\n",time, packet_from_A.seqnum,packet_from_A.payload );

  //printf("(Sender) (in A_output) Packet %d sent.\n", packet_from_A.seqnum);
  
  //as packet is sent, go to next state in sequence.
  //printf("\n(Sender) (in A_output) seqnum :%d,  prev state :%d \n",packet_from_A.seqnum, A_SEQ);
  
  A_SEQ = (A_SEQ == 0)? 1: 3;
  //printf("(Sender) (in A_output) Now going to state %d\n", A_SEQ);
  
  //printf("(Sender) (in A_output) seqnum :%d, state :%d \n\n",packet_from_A.seqnum, A_SEQ);  
  return;

}

/* need be completed only for extra credit */
void B_output(struct msg message)
{
  printf("in B_output\n"); 
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  /*printf("in A_input\n\n");
  printf("  (Sender) (in A_input) State is %d\n", A_SEQ);
  printf("  (Sender) (in A_input) ACK received: %d\n", packet.acknum);
 */
  int checksum = calc_CheckSum(packet);
  /*printf("  (Sender) (in A_input) Calculated Checksum: %d\n", checksum);
  printf("  (Sender) (in A_input) Received Checksum: %d\n", packet.checksum);
  */
  // if checksum does not match return
  if (checksum != packet.checksum)
  {
    //printf("  (Sender) (in A_input) Checksum does not match. Received ACK is corrupt\n");
    return;
  }

  // if we get the wrong ack, from the one we are expecting, return
  if ( (A_SEQ == 1 && packet.acknum != 0) || (A_SEQ == 3 && packet.acknum != 1) )
  {
  //   printf("  (Sender) (in A_input) Wrong ACK %d received.\n", packet.acknum);

    return;   
  }
  
  //we got the correct ACK, and it is not corrupt.
  //stop timer and change state
  //printf("  (Sender) (in A_input) Packet % d received successfully at Receiver\n", packet.acknum);
  printf("**** At %f in A_input() : ACK received for pkt%d %s ****\n",time, packet.seqnum,packet.payload );

  stoptimer(0);

  //printf("\n  (Sender) (in A_input) seqnum :%d, ack : %d, prev state :%d \n",packet.seqnum, packet.acknum, A_SEQ);
  
  A_SEQ = (A_SEQ == 1)? 2 : 0;
  //printf("  (Sender) (in A_input) Going to state %d\n", A_SEQ);
  
  //printf("  (Sender) (in A_input) seqnum :%d, state :%d \n\n",packet.seqnum, A_SEQ);
  
  return;
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
 //just resend the packet previously sent, when timer goes off
  //printf("(A_timerinterrupt) Resending .. %.*s \n", 20, packet_from_A.payload);

  tolayer3(0, packet_from_A);
 
  //start the timer again
  starttimer(0, TIMEOUT);
  return;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
  //printf("in A_init\n");
  A_SEQ = 0; 
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  /*printf("\n\n");
  printf("in B_input\n"); 
    
  printf("(in B_input) State is : %d \n", B_ACK);  
  printf("(in B_input) Msg from packet received is : %s \n", packet.payload);
  printf("(in B_input) Seqnum of received packet : %d\n", packet.seqnum);
*/
  //got the packet now making sure if its corrupted or not
  int checksum = calc_CheckSum(packet);
  /*printf("(in B_input) Calculated Checksum: %d\n", checksum);
  printf("(in B_input) Received Checksum: %d\n", packet.checksum);
*/
  memset(&ack_pkt_from_B, 0, sizeof(ack_pkt_from_B));

  // if we get the wrong packet from the one we are expecting, resend precious ACK

  if ( (B_ACK == 0 && packet.seqnum != 0) || (B_ACK == 1 && packet.seqnum != 1) )
  {
    //printf("(in B_input) Received seqnum %d, expected seqnum %d. Does not match. \n", packet.seqnum, B_ACK);
    
    ack_pkt_from_B.acknum = (B_ACK == 0)? 1: 0; //B_ACK is current ack, since pkt is corrupted we send previous ack so reversing

    ack_pkt_from_B.checksum = calc_CheckSum(ack_pkt_from_B);
    printf("**** At %f in B_input() : Resending ACK for pkt%d %s ****\n",time, packet.seqnum,packet.payload );

   /* printf("(in B_input) Resending ACK %d\n", ack_pkt_from_B.acknum);
    printf("Sending ack packet to layer3 of sender bcz of wrong recv pkt\n");
   */ 
    tolayer3(1, ack_pkt_from_B);  

    return;  
  }

  // if checksum does not match resend the previous ACK
  if (checksum != packet.checksum)
  {
    /*printf("(in B_input) Checksum does not match. Received packet is corrupt \n");
    printf("Received packet's checksum %d | expected checksum %d\n", packet.checksum, checksum);
*/
    ack_pkt_from_B.acknum = (B_ACK == 0)? 1: 0; //B_ACK is current ack, since pkt's checksum doesnt match we send previous ack so reversing


    ack_pkt_from_B.checksum = calc_CheckSum(ack_pkt_from_B);
    /*printf("(in B_input) Resending ACK %d\n", ack_pkt_from_B.acknum);
    printf("Sending ack packet to layer3 of sender bcz of wrong checksum\n");
    */
    tolayer3(1, ack_pkt_from_B);  

    return;  

  }

  // we got the correct packet, and it is not corrupt. generate ACK and send
  printf("**** At %f in B_input() : Correct pkt received. pkt%d %s ****\n",time, packet.seqnum,packet.payload );


  /*printf("(in B_input) Msg from packet received: %s \n", packet.payload);
  printf("(in B_input) Sequence number: %d \n", packet.seqnum);
  printf("(in B_input) Checksum matches.\n");

  printf("Sending to layer 5 of receiver \n");*/
  tolayer5(1, packet.payload);

  ack_pkt_from_B.acknum = (B_ACK == 0)? 0 : 1;
  
  ack_pkt_from_B.checksum = calc_CheckSum(ack_pkt_from_B);
  //printf("(in B_input) Sending ACK %d to A\n", ack_pkt_from_B.acknum);
  
  tolayer3(1, ack_pkt_from_B);

  //next ack will be...
  //printf("\n(in B_input) seqnum :%d, ack : %d, prev state :%d \n",packet.seqnum, packet.acknum, B_ACK);
  
  B_ACK =  (B_ACK == 0)? 1 : 0; 
  //printf("(in B_input) Going to state: %d\n", B_ACK);
  
  //printf("(in B_input) seqnum :%d, state :%d \n\n",packet.seqnum, B_ACK);
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    //printf("in B_init\n");
    B_ACK = 0;
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
//float time = 0.000;
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
            printf(" entity: %d\n", eventptr->eventity);
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
                    /*printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                       // {printf("%c", msg2give.data[i]);
                    printf("\n");*/
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
            if (eventptr->eventity == A) //for abp always goes to if
                A_timerinterrupt();
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
    printf("\n Simulator terminated at time %f\n after sending %d msgs from layer5\n",time, nsim);
}

void init() /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    //nsimmax = 2; lossprob = 0.0; corruptprob =0.0; lambda=50; TRACE = 4;
    nsimmax = 10; lossprob = 0.1; corruptprob =0.3; lambda=1000; TRACE = 4;
    printf("-----  Alternating Bit Protocol -------- \n\n");
    printf("number of messages to simulate: %d\n",nsimmax);
    printf("packet loss probability : %f\n",lossprob);
    printf("packet corruption probability : %f\n",corruptprob);
    printf("average time between messages from sender's layer5 : %f\n",lambda);
    printf("TRACE: %d\n",TRACE);

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
    scanf("%d",&TRACE);
*/
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

void insertevent(struct event *p) //linked list of events
{
    //printf("\n");
    struct event *q, *qold;

    if (TRACE > 2)
    {
        //printf("            INSERTEVENT: time is %lf\n", time);
        //printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
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
    //printf("\n");

}

void printevlist(void)
{
    printf("\n");

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
        //printf("          STOP TIMER: stopping timer at %f\n", time);
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
        //printf("            START TIMER: starting timer at %f\n", time);
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
    //printf("\n");
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
            printf("            TOLAYER3: packet being lost\n");
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
        //printf("            TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,mypktptr->acknum, mypktptr->checksum);
        /*for (i = 0; i < 20; i++)
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
            printf("            TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        /*printf("            TOLAYER3: scheduling arrival on other side\n");
        printf("\n");*/

    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    //printf("\n");
    int i;
    if (TRACE > 2)
    {
        /*printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");*/
    }
    //printf("\n");

}