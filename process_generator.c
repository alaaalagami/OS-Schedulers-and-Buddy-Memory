#include "headers.h"

void clearResources(int);
void sendProcess(int); 
void endProgram(int); 
const Node NULLNODE;
Node *arrivedProc;
queue* allProcesses; 
int time; // stores current time 
int schedulerPid; // stores the PID of the scheduler
int shmidAP; 

// Function to read incoming processes file, save it into a queue, and return the queue
queue* readFile(char *f)
{
   int i, a, b, p;
   queue *Q = (queue*)malloc(sizeof(queue));
   FILE *fp = fopen (f, "r");
   char line[100] = {0};
   char *ignore = fgets (line, 100, fp);
   Node* n;
   while(fscanf(fp, "%d %d %d %d", &i, &a, &b, &p) == 4)
   {    
    n = newNode(i, a, b, p);
    enqueue(Q, n); 
   }
   fclose(fp);
   return Q; 
}

int main(int argc, char * argv[])
{
 //   signal(SIGINT, clearResources); // Clears resources when interrupted 
    signal(SIGUSR1, sendProcess); // Sends arrived processes to scheduler each second when signaled by scheduler
    signal(SIGUSR2, endProgram); // Test if all processes are sent to scheduler and end the program when signaled by scheduler
    char file[100]; // name of the input file where processes info is stored 
    int scheduler; // scheduler type
    int quantum = 0; // scheduler time quantum if round robin (remains 0 otherwise)
    
    // Read input file
    printf("You come again, kind user! \nEnter the name of your processes file:");
    scanf("%s", file); 
    allProcesses = readFile(file); 

    // Choose scheduler type
    printf( "Pick your scheduler: \n 1. Preemptive Highest Priority First \n 2. Shortest Job First \n 3. Round Robin \n 4. Shortest Remaining Time Next \nThe number of your choice: "); 
    scanf("%d", &scheduler); 

    // If chosen scheduler is round robin, input time quantum
    if (scheduler == 3)
    {
     printf("The length of your quantum in seconds: ");
     scanf("%d", &quantum);
    }
   
    // Shared memory to send arriving processes to scheduler
    shmidAP = shmget(59, sizeof(Node*), IPC_CREAT|0644);

    //arrivedProc = (Node*)shmat(shmidAP, NULL, 0); //WHY ARE WE READING HERE, WE JUST CREATED THE MEMORY

 //   if(arrivedProc == (void*)-1)
 //   {
 //       perror("Error in attach in process generator AP");
 //       exit(-1);
 //   }

    if(shmidAP == -1)
    {
        perror("Error in creating shmidAP");
        exit(-1);
    }

    // Create clock
   int clockPid = fork();

    if (clockPid == -1)
        perror("error in fork");

    else if (clockPid == 0)
    {
        char *argv[] = { "./clk.out", 0 };
        execve(argv[0], &argv[0], NULL);
    }

    // Initiatize clock
    initClk();
    

    // Initiate scheduler
    schedulerPid = fork();


    if (schedulerPid == -1)
        perror("error in fork");

    else if (schedulerPid == 0)
    {
        printf("d5lt\n");
        char s[4];
        char q[4];
        snprintf(s, 4,"%d", scheduler);
        snprintf(q, 4, "%d", quantum); 
        char *argv[] = { "./scheduler.out", s, q, 0 };// q is zero if not round robin 
        execve(argv[0], &argv[0], NULL);
    }
    pause();

    // Initialize time counter
    time = -1;

    //printf("current time is %d\n", startTime);
    Node* n = newNode(0, 0, 0, 0);
    while(1)
    {
      // If 1 clock second passes, increment internal time counter and check queue of all processes
      if (getClk() > time)
        {
          time++;
          arrivedProc = (Node*)shmat(shmidAP, NULL, 0); 
          // If any process arrived at this second, add it to arrived process shared memory and signal scheduler to execute second
          if (!isEmpty(allProcesses) && peek(allProcesses) == time)
          {
           n = dequeue(allProcesses);
           *arrivedProc = *n;
          }
          // If no process arrived at current second, signal scheduler to execute second 
          else 
          {
            *arrivedProc = NULLNODE;
          }   
          shmdt(arrivedProc); 
          kill(schedulerPid, SIGUSR1);
        }
    }

    return 0; 

}

// Clear resources when interrupted 
//void clearResources(int signum)
//{
//    shmctl(shmidAP, IPC_RMID, NULL);
//    destroyClk(true);
//}

// Sends arrived processes to scheduler each second when signaled by scheduler
void sendProcess(int signum)
{
    arrivedProc = (Node*)shmat(shmidAP, NULL, 0); 
    if (!isEmpty(allProcesses) && peek(allProcesses) == time)
    {
     Node* n = dequeue(allProcesses);
     *arrivedProc = *n;
    }
    // If no other process arrived at current second, signal scheduler to execute second 
    else 
    {
     *arrivedProc = NULLNODE;
    } 
    shmdt(arrivedProc);
    kill(schedulerPid, SIGUSR1);
}


// Test if all processes are sent to scheduler and end the program when signaled by scheduler
void endProgram(int signum)
{
    if (isEmpty(allProcesses))
    {
      shmctl(shmidAP, IPC_RMID, NULL);
      destroyClk(true);
      kill(schedulerPid, SIGINT);
      raise(SIGINT);
    } 
}



