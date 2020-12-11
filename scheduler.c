#include "headers.h"
const Node NULLNODE;

void executeSecond(int); 
void endScheduler(int);
void freeScheduler(int);
void readRemainingTime(int);
void printProcess(Node*, char[]);

int scheduler; // Stores scheduler type (1 = HPF, 2 = SJF, 3 = RR, 4 = SRTN)
int quantum;
int quantumCounter;
int shmidAP;
Node *arrivedProc; 
int shmidRT; 
bool isBusy; // Flag to know if there is a current running process
Node* runningProcess;
int numProcesses;

// Data structures to be used by the different scheduler types to store PCBs 
queue *Q;
pqueue *pQ;
bqueue *bQ; 
rqueue *rQ;
queue *finishedQ; // To keep finished processes

FILE *logFile;

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
    runningProcess = (Node*)malloc(sizeof(Node));
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
    shmidRT = shmget(49, sizeof(int*), IPC_CREAT|0644);
    //int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
    //if(shmaddr == (void*)-1)
    //{
    //    perror("Error in attach in scheduler RT");
    //    exit(-1);
    //}
    quantumCounter = 0;
    numProcesses = 0;
    logFile = fopen ("Scheduler.log", "w");
    fprintf(logFile, "#At\ttime\tx\tprocess\ty\tstate\t\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    
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

    // Add arrived process to scheduler's working queue
    if (arrivedProc->ID > 0)
    {
      numProcesses++;
      if (scheduler == 1){
        ppush(pQ, newNode(arrivedProc->ID,arrivedProc->arrivalTime,arrivedProc->burstTime, arrivedProc->priority));
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
      }
      else if (scheduler == 2){
        bpush(bQ, newNode(arrivedProc->ID,arrivedProc->arrivalTime,arrivedProc->burstTime, arrivedProc->priority));
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
      } 
      else if (scheduler == 3){
        enqueue(Q, newNode(arrivedProc->ID,arrivedProc->arrivalTime,arrivedProc->burstTime, arrivedProc->priority));
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
      }
      else{
        rpush(rQ, newNode(arrivedProc->ID,arrivedProc->arrivalTime,arrivedProc->burstTime, arrivedProc->priority));
        printf("%d: Pushed process #%d! \n",getClk(),arrivedProc->ID);
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
  isBusy = false;
  runningProcess->finishingTime = getClk();
  Node* myNode = newNode(runningProcess->ID, runningProcess->arrivalTime, runningProcess->burstTime, runningProcess->priority);
  myNode->finishingTime = getClk();
  myNode->remainingTime = runningProcess->remainingTime;
  myNode->waitingTime = runningProcess->waitingTime;
  enqueue(finishedQ, myNode);

  fprintf(logFile, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), 
        runningProcess->ID, runningProcess->arrivalTime, runningProcess->burstTime, 0, runningProcess->waitingTime, 
        getClk() - runningProcess->arrivalTime, (double)(getClk() - runningProcess->arrivalTime) / runningProcess->burstTime);
  
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
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void readRemainingTime(int signum){
  int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
  runningProcess->remainingTime = *shmaddr;
  printProcess(runningProcess, "stopped");
  Node* myNode = newNode(runningProcess->ID, runningProcess->arrivalTime, runningProcess->burstTime, runningProcess->priority);
  myNode->pid = runningProcess->pid;
  myNode->waitingTime = runningProcess->waitingTime;
  myNode->remainingTime = runningProcess->remainingTime;
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

