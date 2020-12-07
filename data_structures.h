#include <stdio.h> 
#include <stdlib.h> 

// Define process node
typedef struct node { 
    int ID; 
    int arrivalTime;
    int burstTime; 
    int priority;
    int remainingTime; // only necassary when using SRTN, initially set as burst time, then changed for SRTN processes as they proceed
    int finishTime; // to be set by scheduler when the process exits
    int status; // -1 = not arrived, 1 = running, 2 = waiting
    int pid; // to be set when process is forked from scheduler
  
    struct node* next; 
  
} Node;

Node* newNode(int id, int at, int bt, int p) 
{ 
    Node* n = (Node*)malloc(sizeof(Node)); 
    n->ID = id;
    n->arrivalTime = at; 
    n->burstTime = bt;  
    n->priority = p; 
    n->remainingTime = bt;
    n->finishTime = -1; 
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
    newnode->next = q->head;
    q->head = newnode;
}

Node* dequeue(queue *q) 
{

    if (q->head == NULL) return NULL;

    Node *current, *prev = NULL;    
    current = q->head;
    while (current->next != NULL) {
        prev = current;
        current = current->next;
    }
 
    if (prev)
    {
        prev->next = NULL;
    }
    else
    {
        q->head = NULL;
    }
    return current;
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
    Node* temp = q->head; 
    q->head = (q->head)->next; 
    return temp; 
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
    Node* temp = q->head; 
    q->head = (q->head)->next; 
    return temp; 
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
    Node* temp = q->head; 
    q->head = (q->head)->next; 
    return temp; 
}  

