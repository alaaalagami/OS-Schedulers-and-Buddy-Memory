#include "headers.h"

int *remainingtime;

void sendRT(int signum); 

int main(int agrc, char * argv[])
{
    signal(SIGSTOP, sendRT); // To place remaining time in shared memory with scheduler when stopped 
    // Initializes clock and loops till 1 clock second passes while remaining time for the process is greater than 0
    initClk(); 
    int timeNow = getClk();
    remainingtime = (int*)argv[1];
    while (remainingtime > 0)
    {
        // If 1 second passes, increments time counter and decrements remaining time 
        if (getClk() > timeNow) 
        {
          timeNow++; 
          remainingtime--; 
        }
    }


    // When remaining time ends, a process detaches from clock and sends its PID as exit code to the scheduler
    destroyClk(false);
    int myPid = getpid(); 
    exit(myPid); 
    
    return 0;
}

// Sends remaining time back to scheduler when stopped by scheduler 
void sendRT(int signum)
{
    int shmidRT = shmget(49, sizeof(int*), 0);
    int *shmaddr = (int*)shmat(shmidRT, NULL, 0);
    if(shmaddr == (void*)-1)
    {
        perror("Error in attach in process");
        exit(-1);
    }
    shmaddr = remainingtime;
    shmdt(shmaddr);

    // If remaining time = 0 process exits
    if (remainingtime == 0)
    {
       destroyClk(false);
       int myPid = getpid(); 
       exit(myPid);
    }

    signal(SIGSTOP, SIG_DFL);
    raise(SIGSTOP);
}

