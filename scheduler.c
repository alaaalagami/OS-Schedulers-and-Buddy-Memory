#include "headers.h"

void executeSecond(int); 
void endScheduler(int);
int scheduler; // Stores scheduler type (1 = HPF, 2 = SJF, 3 = RR, 4 = SRTN)
int shmidAP;
Node *arrivedProc; 
int shmidRT; 

// Data structures to be used by the different scheduler types to store PCBs 
queue *Q;
pqueue *pQ;
bqueue *bQ; 
rqueue *rQ;
queue *finishedQ; // To keep finished processes

int main(int argc, char * argv[])
{
    initClk();
    scheduler = atoi(argv[1]); // scheduler type
    int quantum = atoi(argv[2]); // time quantum if using Round Robin
    printf("%d %d\n", scheduler, quantum); // Just here to test when the program fails and the program fails even before coming here :)

    signal(SIGUSR1, executeSecond); // Takes incoming arrived process from process_generator and excutes the second's scheduling
    signal(SIGINT, endScheduler); // Ends scheduler when interrupted or ended by process_generator 

    // Define queues to be used 
    Q = (queue*)malloc(sizeof(queue));
    pQ = (pqueue*)malloc(sizeof(pqueue));
    bQ = (bqueue*)malloc(sizeof(bqueue));
    rQ = (rqueue*)malloc(sizeof(rqueue));
    finishedQ = (queue*)malloc(sizeof(queue)); // To keep finished processes

    // Shared memory to get arriving processes
    shmidAP = shmget(59, sizeof(Node*), 0);
    //WHY ARE WE ATTACHING HERE?
    //arrivedProc = (Node*)shmat(shmidAP, NULL, 0);
    //if(arrivedProc == (void*)-1)
    //{
    //    perror("Error in attach in scheduler AP");
    //    exit(-1);
    //}
    if(shmidAP == -1)
    {
        perror("Error in getting shmidAP");
        exit(-1);
    }
    
    // Shared memory for stopped process to place its remaining time
    //shmidRT = shmget(49, sizeof(int*), IPC_CREAT|0644);
    //int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
    //if(shmaddr == (void*)-1)
    //{
    //    perror("Error in attach in scheduler RT");
    //    exit(-1);
    //}

    while (1)
    {

    }
}

// Takes incoming arrived process from process_generator and excutes the second's scheduling
void executeSecond(int signum)
{
    // WE SHOULD ATTACH HERE
    arrivedProc = (Node*)shmat(shmidAP, NULL, 0);
    if(arrivedProc == (void*)-1)
    {
        perror("Error in attach in scheduler AP");
        exit(-1);
    }
    printf("Schedule Attached \n");

    // Add arrived process to scheduler's working queue
    if (arrivedProc->ID > 0)
    {
      printf("Not NULL \n");
      printf("I am going to schedulers now \n");

      if (scheduler == 1){
        printf("I am going to scheduler 1 now \n");
        ppush(pQ, arrivedProc); 
        printf("Pushed! \n");
      }
      else if (scheduler == 2)
        bpush(bQ, arrivedProc); 
      else if (scheduler == 3)
        enqueue(Q, arrivedProc); 
      else
        rpush(rQ, arrivedProc);

      printf("I am going to trigger the signal \n");
      kill(getppid(), SIGUSR1); // Requests other arrived processes if any
    }
  else // if no more processes arrived, scheduler executes second's actions
   {
      printf("IS NULL \n");
      //////////////////////// SCHEDULING GOES HERE ///////////////////////
      // if's to test if queue of arrived is empty and no process is running
      // kill(getppid(), SIGUSR2); // Calls process generator to test if all processes have arrived to scheduler and end program
    
      // Note: to make a new Node pointer call newNode(int id, int at, int bt, int p) (check data_structires.h for node and DS details)
      // Note 2: when exiting free() all pointers (zy delelte fe cpp)
    }
    //WE SHOULD DETACH HERE
    printf("Schedule Detached \n");
    shmdt(arrivedProc);
}

// Ends scheduler when interrupted or ended by process_generator 
void endScheduler(int signum)
{
    destroyClk(true);
    shmctl(shmidRT, IPC_RMID, NULL);
    // Create perf file
}


