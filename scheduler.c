#include "headers.h"
const Node NULLNODE;

void executeSecond(int); 
void endScheduler(int);
void freeScheduler(int);
void readRemainingTime(int);
void printProcess(Node*, char[]);
bool allocateMemory(Node *Proc);
void deallocateMemory(Node *Proc);

int scheduler; // Stores scheduler type (1 = HPF, 2 = SJF, 3 = RR, 4 = SRTN)
int quantum;
int quantumCounter;
int shmidAP;
Node *arrivedProc; 
int shmidRT; 
bool isBusy; // Flag to know if there is a current running process
Node* runningProcess;
int numProcesses;
int msgqid;
int shmidReturnedP; 
struct msgbuff message;


// Data structures to be used by the different scheduler types to store PCBs 
queue *Q;
pqueue *pQ;
bqueue *bQ; 
rqueue *rQ;
queue *finishedQ; // To keep finished processes
queue *unallocatedQ;
queue *tempQ;

// Define memory
memory *M[11]; 

// Log files for scheduler and memory
FILE *logFile;
FILE *memFile; 

int main(int argc, char * argv[])
{
    initClk();
    int time = getClk();
    scheduler = atoi(argv[1]); // scheduler type
    quantum = atoi(argv[2]); // time quantum if using Round Robin
    printf("%d %d\n", scheduler, quantum); // Just here to test when the program fails and the program fails even before coming here :)

    signal(SIGUSR1, executeSecond); // Takes incoming arrived process from process_generator and excutes the second's scheduling
    signal(SIGINT, endScheduler); // Ends scheduler when interrupted or ended by process_generator 
    signal(SIGUSR2, freeScheduler);
    signal(SIGHUP, readRemainingTime);

    isBusy = false; // Initially no processes are running

    // Define queues to be used 
    Q = (queue*)malloc(sizeof(queue));
    pQ = (pqueue*)malloc(sizeof(pqueue));
    bQ = (bqueue*)malloc(sizeof(bqueue));              
    rQ = (rqueue*)malloc(sizeof(rqueue));
    finishedQ = (queue*)malloc(sizeof(queue)); // To keep finished processes
    unallocatedQ = (queue*)malloc(sizeof(queue));
    tempQ = (queue*)malloc(sizeof(queue));
    runningProcess = (Node*)malloc(sizeof(Node));

    for(int i = 0; i<11; i++)
      M[i] = (memory*)malloc(sizeof(memory)); 

    // Allocate first empty memory partition 
    memNode * m = newmemNode(0); // Empty memory block of size 1024 bytes starting at index 0
    M[10]->head = m;

    // Shared memory to get arriving processes
    shmidAP = shmget(59, sizeof(Node*), 0);

    if(shmidAP == -1)
    {
        perror("Error in getting shmidAP");
        exit(-1);
    }
    
    // Shared memory for stopped process to place its remaining time
    shmidRT = shmget(49, sizeof(int*), IPC_CREAT|0644);
    //int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
    //if(shmaddr == (void*)-1)
    //{
    //    perror("Error in attach in scheduler RT");
    //    exit(-1);
    //}

    // Message bufer to send returned processes that do not fit into scheduler memory
    //msgqid = msgget(303, 0); 
    //if(msgqid == -1)
    //{
    //    perror("Error in create");
    //    exit(-1);
    //}
    // Shared memory to store number of returned processes
    shmidReturnedP = shmget(501, 4, 0);
    if ((long)shmidReturnedP == -1)
    {
        perror("Error in creating shm!");
        exit(-1);
    }
    quantumCounter = 0;
    numProcesses = 0;
    logFile = fopen ("Scheduler.log", "w");
    fprintf(logFile, "#At\ttime\tx\tprocess\ty\tstate\t\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    
    memFile = fopen ("memory.log", "w");
    fprintf(memFile, "#At\ttime\tx\tallocated\ty\tbytes\tfor\tprocess\tz\tfrom\ti\tto\tj\n");
  
    kill(getppid(), SIGUSR1);
    while (1)
    {

    }
}

// Takes incoming arrived process from process_generator and excutes the second's scheduling
void executeSecond(int signum)
{
    // WE SHOULD ATTACH HERE
    Node* n; // Spare Node pointer
    arrivedProc = (Node*)shmat(shmidAP, NULL, 0);
    if(arrivedProc == (void*)-1)
    {
        perror("Error in attach in scheduler AP");
        exit(-1);
    }
    
    printf("Checking unallocated processes....\n");
    while(getHead(unallocatedQ)){
      n = dequeue(unallocatedQ);
      n->next = NULL;
      bool allocated = allocateMemory(n);
      if (allocated){
        if (scheduler == 1){
          ppush(pQ, n);
          printf("%d: Pushed process #%d! \n",getClk(),n->ID);
        }
        else if (scheduler == 2){
         bpush(bQ, n);
         printf("%d: Pushed process #%d! \n",getClk(),n->ID);
        } 
        else if (scheduler == 3){
         enqueue(Q, n);
         printf("%d: Pushed process #%d! \n",getClk(),n->ID);
        }
        else{
         rpush(rQ, n);
         printf("%d: Pushed process #%d! \n",getClk(),n->ID);
       }
      }
      else{
  //      printf("Putting %d in tempQ\n", n->ID);
        enqueue(tempQ, n);
      }
    }

    while(getHead(tempQ)){
       n = dequeue(tempQ);
 //      printf("Entered to get %d!\n", n->ID);
       n->next = NULL;
       enqueue(unallocatedQ, n);
 //      printf("Putting %d back in unallocatedQ\n", getHead(unallocatedQ)->ID);
    }


    // Add arrived process to scheduler's working queue
    if (arrivedProc->ID > 0)
    {
      printf("%d: Process #%d Arrived! \n",getClk(),arrivedProc->ID);
      numProcesses++;
      Node* recieved = newNode(arrivedProc->ID,arrivedProc->arrivalTime,arrivedProc->burstTime, arrivedProc->priority, arrivedProc->memsize);
      // Allocate in memory
      bool allocated = allocateMemory(recieved); 
      //bool allocated = 1;  
      // If allocated, place in scheduler waiting queue
      if (allocated) 
      {
       if (scheduler == 1){
        ppush(pQ, recieved);
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
       }
       else if (scheduler == 2){
        bpush(bQ, recieved);
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
       } 
       else if (scheduler == 3){
        enqueue(Q, recieved);
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
       }
       else{
        rpush(rQ, recieved);
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
       }
      }
      else{
        enqueue(unallocatedQ, recieved);
      }

      kill(getppid(), SIGUSR1); // Requests other arrived processes if any
    }
  else // if no more processes arrived, scheduler executes second's actions
   {
      if (scheduler == 1 && !pisEmpty(pQ)){

        if (runningProcess->ID > 0 && ppeek(pQ) < runningProcess->priority){ // Preemption
          kill(runningProcess->pid, SIGUSR2);
          pause();
        }

        if (!isBusy){
          isBusy = true;
          n = ppop(pQ); 
          runningProcess = n;
          if (n->pid == -1){ // Process not initiated yet
             int processPid = fork();

             if (processPid == -1)
               printf("Error in forking process #%d\n", n->ID);

             else if (processPid == 0)
              {
               printf("%d: Process #%d ready for action!\n",getClk(),n->ID);
               char remainingTime[4];
               char ID[4];
               snprintf(remainingTime, 4,"%d", n->remainingTime);
               snprintf(ID, 4,"%d", n->ID);
               char *argv[] = { "./process.out", remainingTime, ID, 0 };
               execve(argv[0], &argv[0], NULL);
              }
              runningProcess->pid = processPid;
              printProcess(runningProcess, "started");
          }
          else // Process is continued not initiated
          {
              printf("%d: Process #%d ready for MORE action!\n",getClk(), n->ID);
              printProcess(n, "resumed");
              kill(n->pid, SIGCONT);
          }
        }

        pincrementWaiting(pQ);    
      }
      else if (scheduler == 2 && !bisEmpty(bQ)){
        if (!isBusy){
          isBusy = true;
          n = bpop(bQ);
          runningProcess = n;
          int processPid = fork();
          if (processPid == -1)
               printf("Error in forking process #%d\n", n->ID);
          else if (processPid == 0)
              {
               printf("%d: Process #%d ready for action!\n",getClk(),n->ID);
               char remainingTime[4];
               char ID[4];
               snprintf(remainingTime, 4,"%d", n->remainingTime);
               snprintf(ID, 4,"%d", n->ID);
               char *argv[] = { "./process.out", remainingTime, ID, 0 };
               execve(argv[0], &argv[0], NULL);
              }

          runningProcess->pid = processPid;
          printProcess(runningProcess, "started");
        }
        bincrementWaiting(bQ);
      }
      else if (scheduler == 3){
        if (isBusy){
          quantumCounter++;
          if (quantumCounter == quantum){ // The quantum has passed
            if (getHead(Q)){ // There are other waiting processes
              kill(runningProcess->pid, SIGUSR2);
              pause();
              quantumCounter = 0;
              }
            else{ // There are no other processes so give the processes another quantum
            if (quantumCounter == quantum){
              quantumCounter = 0;
            }
            }
          }
       }

        if (!isBusy && !isEmpty(Q)){ // Scheduler is free
             isBusy = true;
            // quantumCounter = 0;
             n = dequeue(Q); 
             runningProcess = n;
            if (n->pid == -1){ // Process not initiated yet
               int processPid = fork();

               if (processPid == -1)
                 printf("Error in forking process #%d\n", n->ID);
               else if (processPid == 0)
                {
                 printf("%d: Process #%d ready for action!\n",getClk(),n->ID);
                 char remainingTime[4];
                 char ID[4];
                 snprintf(remainingTime, 4,"%d", n->remainingTime);
                 snprintf(ID, 4,"%d", n->ID);
                 char *argv[] = { "./process.out", remainingTime, ID, 0 };
                 execve(argv[0], &argv[0], NULL);
                }
              runningProcess->pid = processPid;
              printProcess(runningProcess, "started");
             }
            else // Process is continued not initiated
            {
              printf("%d: Process #%d ready for MORE action!\n",getClk(), n->ID);
              quantumCounter = 0;
              printProcess(n, "resumed");
              kill(n->pid, SIGCONT);
            }
          }
        incrementWaiting(Q);
      }
      else if (scheduler == 4 && !risEmpty(rQ)){
        if (runningProcess->ID > 0 && rpeek(rQ) < runningProcess->remainingTime){ // Preemption
          kill(runningProcess->pid, SIGUSR2);
          pause();
        }

        if (!isBusy){
          isBusy = true;
          n = rpop(rQ); 
          runningProcess = n;
          if (n->pid == -1){ // Process not initiated yet
             int processPid = fork();

             if (processPid == -1)
               printf("Error in forking process #%d\n", n->ID);
             else if (processPid == 0)
              {
               printf("%d: Process #%d ready for action!\n",getClk(),n->ID);
               char remainingTime[4];
               char ID[4];
               snprintf(remainingTime, 4,"%d", n->remainingTime);
               snprintf(ID, 4,"%d", n->ID);
               char *argv[] = { "./process.out", remainingTime, ID, 0 };
               execve(argv[0], &argv[0], NULL);
              }

              runningProcess->pid = processPid;
              printProcess(runningProcess, "started");
          }
          else // Process is continued not initiated
          {
              printf("%d: Process #%d ready for MORE action!\n",getClk(), n->ID);
              printProcess(n, "resumed");
              kill(n->pid, SIGCONT);
          } 
        }
        rincrementWaiting(rQ);
      }
    }
    //WE SHOULD DETACH HERE
    shmdt(arrivedProc);

    if (!getHead(Q) && !pgetHead(pQ) && !bgetHead(bQ) && !rgetHead(rQ) && !isBusy){
      kill(getppid(), SIGUSR2);
    }
}

// Frees the scheduler when a process is finished 
void freeScheduler(int signum)
{
printf("freeing %d\n", runningProcess->ID);

  isBusy = false;
  runningProcess->finishingTime = getClk();
  Node* myNode = newNode(runningProcess->ID, runningProcess->arrivalTime, runningProcess->burstTime, runningProcess->priority, runningProcess->memsize);
  myNode->finishingTime = getClk();
  myNode->remainingTime = runningProcess->remainingTime;
  myNode->waitingTime = runningProcess->waitingTime;
  enqueue(finishedQ, myNode);

  fprintf(logFile, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), 
        runningProcess->ID, runningProcess->arrivalTime, runningProcess->burstTime, 0, runningProcess->waitingTime, 
        getClk() - runningProcess->arrivalTime, (double)(getClk() - runningProcess->arrivalTime) / runningProcess->burstTime);

  // Deallocate memory
  deallocateMemory(runningProcess); 
  
  *runningProcess = NULLNODE;
  quantumCounter = 0;

 
}


// Ends scheduler when interrupted or ended by process_generator 
void endScheduler(int signum)
{
    destroyClk(true);
    shmctl(shmidRT, IPC_RMID, NULL);

    // Create perf file
    FILE* perfFile = fopen("Scheduler.perf", "w");
    fprintf(logFile, "#At\ttime\tx\tprocess\ty\tstate\t\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    Node* n;
    double totalTA = 0, totalWTA = 0, stdWTA = 0, totalWaiting = 0;
    double* WTAs = (double*) calloc (numProcesses,sizeof(double));
    int i = 0;
    while(getHead(finishedQ)){
      n = dequeue(finishedQ);
      double TA, WTA;
      TA = n->finishingTime - n->arrivalTime;
      totalTA += TA;
      WTA = (double)(TA)/n->burstTime;
      totalWTA += WTA;
      n->WTA = WTA;
      totalWaiting += n->waitingTime;
      WTAs[i] = WTA;
      i++;
    }

    double meanTA = totalTA / numProcesses;
    double meanWTA = totalWTA / numProcesses;
    double meanWaiting = totalWaiting / numProcesses;

    double sumOfDifferences = 0;

    for(int k = 0; k < i; k++){
      sumOfDifferences += pow(WTAs[i] - meanWTA,2);
    }
    
    stdWTA = sqrt(sumOfDifferences / (double) numProcesses);

    fprintf(perfFile, "CPU Utilization: 100%%\n");
    fprintf(perfFile, "Avg WTA: %.2f\n", meanWTA);
    fprintf(perfFile, "Avg Waiting: %.2f\n", meanWaiting);
    fprintf(perfFile, "Std WTA: %.2f\n", stdWTA);

    fclose(perfFile);
    fclose(logFile);
    fclose(memFile); 
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void readRemainingTime(int signum){
  int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
  runningProcess->remainingTime = *shmaddr;
  printProcess(runningProcess, "stopped");
  Node* myNode = newNode(runningProcess->ID, runningProcess->arrivalTime, runningProcess->burstTime, runningProcess->priority, runningProcess->memsize);
  myNode->pid = runningProcess->pid;
  myNode->waitingTime = runningProcess->waitingTime;
  myNode->remainingTime = runningProcess->remainingTime;
  myNode->memIndex = runningProcess->memIndex; 
  myNode->memPower = runningProcess->memPower;
  isBusy = false;

  if (scheduler == 1){
    printf("%d: Process %d is getting preempted by Process %d. Rem time = %d\n", getClk(), runningProcess->ID, pQ->head->ID, runningProcess->remainingTime);
    ppush(pQ, myNode);
  }
  else if (scheduler == 3){
    printf("%d: Process %d is getting preempted by Process %d. Rem time = %d\n", getClk(), runningProcess->ID, Q->head->ID, runningProcess->remainingTime);
    enqueue(Q, myNode);
  }
  else if (scheduler == 4){
    printf("%d: Process %d is getting preempted by Process %d. Rem time = %d\n", getClk(), runningProcess->ID, rQ->head->ID, runningProcess->remainingTime);
    rpush(rQ, myNode);
  }
}

void printProcess(Node* n, char status[]){
  fprintf(logFile, "At\ttime\t%d\tprocess\t%d\t%s\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), n->ID, status, n->arrivalTime, n->burstTime, n->remainingTime, n->waitingTime);
}



// Memory allocation 
bool allocateMemory(Node *Proc){
  printf("in allocate %d\n", Proc->ID); 
  int pSize = Proc->memsize; 
  int tempSize = pSize; 
  int power = 0;
  int neededSize; 
  int tempIndex; 
  memNode *m; 
  while (tempSize!=1)
  {
    tempSize /=2; 
    power++; 
  };
  neededSize = pow(2,power);  
  if (pow(2,power) != pSize)
  {
    power++; 
    neededSize = pow(2,power);
  }
printf("Needed Size = %d\n", neededSize); 
  Proc->memPower = power;  
 
  int propower = power; 
  while (M[power]->head == NULL && power < 11)
    power++; 

  if (power < 11)
  {
    m = M[power]->head; 
    M[power]->head = m->next;
    tempIndex = m->startIndex;
    Proc->memIndex = tempIndex; 
printf("Start Index = %d\n", Proc->memIndex); 
    while (power > propower)
    {
      power--;
      memNode * newmem = newmemNode(tempIndex + pow(2,power));
      M[power]->head = newmem; 
    }
    fprintf(memFile, "At\ttime\t%d\tallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n", getClk(),Proc->memsize, Proc->ID, tempIndex, tempIndex + (int)pow(2, propower) - 1);
    printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(),Proc->memsize, Proc->ID, tempIndex, tempIndex + (int)pow(2, propower) - 1);
    return 1; 
  } 
  else 
  {
//    int* ReturnedPcount = (int *) shmat(shmidReturnedP, (void *)0, 0);
//    (*ReturnedPcount)++;
//    shmdt(ReturnedPcount);

//    message.mtype = 1; 
//    message.ReturnedP = *Proc;
//    printf("Sent process %d back to generator\n", message.ReturnedP.ID);

//    int send_val = msgsnd(msgqid, (const void *) &message, sizeof(message.ReturnedP), !IPC_NOWAIT);

//    if(send_val == -1)
//        perror("Errror in send");
    printf("Process %d could not be allocated..\n", Proc->ID);
    return 0; 
  }

}

// Memory deallocation 
void deallocateMemory(Node *Proc)
{
  printf("in deallocate %d\n", Proc->ID); 
  int tempSize = Proc->memsize; 
  int tempIndex = Proc->memIndex; 
  int buddyIndex; 
  int startingIndex; 
  int power = Proc->memPower; 
  bool found = 1;
  memNode* m; 
  memNode* p;
  while (found) 
  {
    printf("Deallocating Size = %d starting at %d\n", (int)pow(2, power), tempIndex); 
    if ((tempIndex / (int)pow(2, power))% 2 == 0)
    {
     buddyIndex = tempIndex + pow(2, power); 
     startingIndex = tempIndex;
    }
    else
    {
     buddyIndex = tempIndex - pow(2, power);
     startingIndex = buddyIndex; 
    }

    m = M[power]->head;
    found = 0; 
    while (m!=NULL)
    {
  // printf("%d\n",m->startIndex);
      found = 0; 
      if (m->startIndex==buddyIndex)
      {
       found = 1; 
       if (p)
        p->next = m->next; 
       else
        M[power]->head = m->next;    
       break;
      }
      p = m; 
      m = m->next;
    } 
   // printf("%d\n", found);
    if (!found)
    { 
     memNode * newmem = newmemNode(tempIndex); 
     m = M[power]->head;
     if (m == NULL)
      M[power]->head = newmem;
     else if (m->startIndex >= tempIndex)
     {
       M[power]->head = newmem; 
       newmem->next = m;
     } 
     else
     {
      if (m->next == NULL)
      {m->next = newmem;}
      else {
      p = m; 
      while (m->startIndex < tempIndex)
      {
        p = m; 
        if (m->next == NULL)
         {break;}
        m = m->next; 
      }
      p->next = newmem;
      newmem->next = m->next;}
     }   
    }
    tempIndex = startingIndex; 
    power++; 
    tempSize /= 2; 
  } 
    fprintf(memFile, "At\ttime\t%d\tdeallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n", getClk(),Proc->memsize, Proc->ID, Proc->memIndex, Proc->memIndex + (int)pow(2, Proc->memPower) - 1);
}

