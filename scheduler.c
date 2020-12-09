#include "headers.h"
const Node NULLNODE;

void executeSecond(int); 
void endScheduler(int);
void freeScheduler(int);

int scheduler; // Stores scheduler type (1 = HPF, 2 = SJF, 3 = RR, 4 = SRTN)
int shmidAP;
Node *arrivedProc; 
int shmidRT; 
bool isBusy; // Flag to know if there is a current running process
Node* runningProcess;

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
    signal(SIGUSR2, freeScheduler);

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
          sleep(1);
          int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
          runningProcess->remainingTime = *shmaddr;
            printf("%d: Process %d is getting preempted by Process %d. Rem time = %d\n", getClk(), runningProcess->ID, pQ->head->ID, runningProcess->remainingTime);
            Node* myNode = newNode(runningProcess->ID, runningProcess->arrivalTime, runningProcess->remainingTime, runningProcess->priority);
            myNode->pid = runningProcess->pid;
            ppush(pQ, myNode);
            isBusy = false;
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
          }
          else // Process is continued not initiated
          {
              printf("%d: Process #%d ready for MORE action!\n",getClk(), n->ID);
              kill(n->pid, SIGCONT);
          }
          
        }
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
        }
      }
      else if (scheduler == 3 && !isEmpty(Q)){

      }
      else if (scheduler == 4 && !risEmpty(rQ)){
        if (runningProcess->ID > 0 && rpeek(rQ) < runningProcess->remainingTime){ // Preemption
          printf("OH DAMN WE NEED PREEMPT!!\n");
          kill(runningProcess->pid, SIGUSR2);
          sleep(1);
          printf("Just Woke Up!!\n");
          int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
          printf("Connected to shared memory!!\n");
          runningProcess->remainingTime = *shmaddr;
          printf("%d: Process %d is getting preempted by Process %d. Rem time = %d\n", getClk(), runningProcess->ID, rQ->head->ID, runningProcess->remainingTime);
          Node* myNode = newNode(runningProcess->ID, runningProcess->arrivalTime, runningProcess->remainingTime, runningProcess->priority);
          myNode->pid = runningProcess->pid;
          rpush(rQ, myNode);
          isBusy = false;
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
          }
          else // Process is continued not initiated
          {
              printf("%d: Process #%d ready for MORE action!\n",getClk(), n->ID);
              kill(n->pid, SIGCONT);
          } 
        }
      }
    }
    //WE SHOULD DETACH HERE
    shmdt(arrivedProc);
}

// Frees the scheduler when a process is finished 
void freeScheduler(int signum)
{
  printf("I AM FREEE \n");
  isBusy = false;
  *runningProcess = NULLNODE;
}


// Ends scheduler when interrupted or ended by process_generator 
void endScheduler(int signum)
{
    destroyClk(true);
    shmctl(shmidRT, IPC_RMID, NULL);
    // Create perf file
}


