#include <stdio.h> 
#include <stdlib.h> 

// PROCESS STRUCTURES
// Define process node
typedef struct node { 
    int ID; 
    int arrivalTime;
    int burstTime; 
    int priority;
    int memsize; 
    int memIndex; // starting index of process in memory (indeces run from 0 to 1023), initially set to -1
    int memPower; // stores the log2(memsize) rounded up, initially set to -1
    int remainingTime; // only necassary when using SRTN, initially set as burst time, then changed for SRTN processes as they proceed
    int waitingTime; // counter for the clock cycles a processes waited
    int finishingTime;
    int WTA;
    int status; // -1 = not arrived, 1 = running, 2 = waiting, 3 = finished.
    int pid; // to be set when process is forked from scheduler
  
    struct node* next; 
  
} Node;

Node* newNode(int id, int at, int bt, int p, int m) 
{ 
    Node* n = (Node*)malloc(sizeof(Node)); 
    n->ID = id;
    n->arrivalTime = at; 
    n->burstTime = bt;  
    n->priority = p; 
    n->memsize = m; 
    n->memIndex = -1; 
    n->memPower = -1; 
    n->remainingTime = bt;
    n->waitingTime = 0;
    n->finishingTime = 0;
    n->WTA = 0;
    n->status = -1; 
    n->pid = -1; 
    n->next = NULL; 
  
    return n; 
} 

// Define data structures
// FIFO Queue for process generator, ready and running processes in Round Robin scheduler, and completed processes
typedef struct Queue {
    struct node *head;
}queue; 

Node* getHead(queue *q)
{
    return (q->head);
}

int isEmpty(queue *q) 
{ 
    return (q -> head) == NULL; 
} 

int peek(queue *q) 
{ 
    return (q -> head -> arrivalTime); 
} 

void enqueue(queue *q, Node *newnode) 
{
    if (q->head == NULL) {q->head = newnode; return;}

    Node *current;

    current = q->head;

    while (current->next != NULL) {
        current = current->next;
    }
 
    current->next = newnode;
}

Node* dequeue(queue *q) 
{
    Node* temp = malloc(sizeof(Node*)); 
    temp = q->head;
    q->head = (q->head)->next; 
    return temp;
}

void incrementWaiting(queue *q) 
{
    if (q->head == NULL) return;
    Node *current;
    current = q->head;

    while (current != NULL) {
        current->waitingTime++;
        current = current->next;
    }
}


// Heap by process priority for ready and running processes in preemptive HPF scheduler
typedef struct priorityQueue {
    Node *head; 
} pqueue;

Node* pgetHead(pqueue *q)
{
    return (q->head);
}

int pisEmpty(pqueue *q) 
{ 
    return (q->head) == NULL; 
}

void ppush(pqueue *q, Node *newnode) 
{ 
    if (q->head == NULL){
        q->head = newnode;
        return;
    }

    if ((q->head)->priority > newnode->priority) { 
        newnode->next = q->head; 
        q->head = newnode; 
    } 
    else {         
        Node* start = q->head;
        while (start->next != NULL && start->next->priority < newnode->priority) { 
            start = start->next; 
        } 
        newnode->next = start->next; 
        start->next = newnode; 
    } 
}

Node* ppop(pqueue *q) 
{ 
    Node* temp = malloc(sizeof(Node*)); 
    temp = q->head;
    q->head = (q->head)->next; 
    return temp; 
}  

int ppeek(pqueue *q)
{ 
    return (q -> head -> priority);
} 

void pincrementWaiting(pqueue *q) 
{
    if (q->head == NULL) return;
    Node *current;
    current = q->head;

    while (current != NULL) {
        current->waitingTime++;
        current = current->next;
    }
}

// Heap by burst time for ready processes in SJF scheduler (no preemption so running processes move to completed processes queue right away)
typedef struct burstQueue {
    Node *head; 
} bqueue; 

Node* bgetHead(bqueue *q)
{
    return (q->head);
}

int bisEmpty(bqueue *q) 
{ 
    return (q ->head) == NULL; 
}

void bpush(bqueue *q, Node *newnode) 
{ 
    if (q->head == NULL){
      q->head = newnode;
      return;
    }

    if ((q->head)->burstTime > newnode->burstTime) { 
        newnode->next = q->head; 
        q->head = newnode; 
    } 
    else {         
        Node* start = q->head;
        while (start->next != NULL && start->next->burstTime < newnode->burstTime) { 
            start = start->next; 
        } 
        newnode->next = start->next; 
        start->next = newnode; 
    } 
}

Node* bpop(bqueue *q) 
{ 
    Node* temp = malloc(sizeof(Node*)); 
    temp = q->head;
    q->head = (q->head)->next; 
    return temp; 
}  

int bpeek(bqueue *q) 
{ 
    return (q -> head -> burstTime);
} 

void bincrementWaiting(bqueue *q) 
{
    if (q->head == NULL) return;
    Node *current;
    current = q->head;

    while (current != NULL) {
        current->waitingTime++;
        current = current->next;
    }
}

// Heap by remaining time for ready processes in SRTN scheduler (the running process will be saved individually)
typedef struct remainingQueue {
    Node *head; 
} rqueue; 

Node* rgetHead(rqueue *q)
{
    return (q->head);
}

int risEmpty(rqueue *q) 
{ 
    return (q ->head) == NULL; 
}

void rpush(rqueue *q, Node *newnode) 
{ 
    if (q->head == NULL){
      q->head = newnode;
      return;
    }

    if ((q->head)->remainingTime > newnode->remainingTime) { 
        newnode->next = q->head; 
        q->head = newnode; 
    } 
    else {         
        Node* start = q->head;
        while (start->next != NULL && start->next->remainingTime < newnode->remainingTime) { 
            start = start->next; 
        } 
        newnode->next = start->next; 
        start->next = newnode; 
    } 
}

Node* rpop(rqueue *q) 
{ 
    Node* temp = malloc(sizeof(Node*)); 
    temp = q->head;
    q->head = (q->head)->next; 
    return temp; 
}  

int rpeek(rqueue *q) 
{ 
    return (q -> head -> remainingTime);
} 

void rincrementWaiting(rqueue *q) 
{
    if (q->head == NULL) return;
    Node *current;
    current = q->head;

    while (current != NULL) {
        current->waitingTime++;
        current = current->next;
    }
}

// MEMORY STRUCTURES 
// Define memory node
typedef struct memnode { 

    int startIndex; // index of memory where partition starts (indices range from 0 till 1023

    struct memnode* next; 
  
} memNode;

memNode* newmemNode(int i) 
{ 
    memNode* n = (memNode*)malloc(sizeof(memNode)); 
    n->startIndex = i;
 
    n->next = NULL;   
    return n; 
}

// Define empty memory linked list
typedef struct MemoryDLL {
    struct memnode *head;
} memory; 

memNode* mgetHead(memory *m)
{
    return (m->head);
}

