#include "headers.h"

int remainingtime;
int ID;
int timeNow;

void sendRT(int signum);
void continueTesting(int signum);

int main(int agrc, char * argv[])
{
    signal(SIGUSR2, sendRT); // To place remaining time in shared memory with scheduler when stopped 
    signal(SIGCONT, continueTesting);

    // Initializes clock and loops till 1 clock second passes while remaining time for the process is greater than 0
    initClk(); 
    timeNow = getClk();
    remainingtime = atoi(argv[1]);
    ID = atoi(argv[2]);
    printf("%d: I AM PROCESS #%d and remaining time %d\n", getClk(), ID, remainingtime);

    while (remainingtime > 0)
    {

        // If 1 second passes, increments time counter and decrements remaining time 
        if (getClk() > timeNow) 
        {
          remainingtime--; 
          timeNow = getClk();
          printf("%d: Time left for Process #%d: %d\n", getClk(), ID, remainingtime);
        }
    }
    
    printf("%d: Process #%d time has ended\n", getClk(), ID);
    // When remaining time ends, a process detaches from clock and sends its PID as exit code to the scheduler
    destroyClk(false);
    kill(getppid(), SIGUSR2);
    raise(SIGINT);
    
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

    *shmaddr = remainingtime;
    shmdt(shmaddr);

    kill(getppid(),SIGHUP);

    // If remaining time = 0 process exits
    if (remainingtime == 0)
    {
       destroyClk(false);
       kill(getppid(), SIGUSR2);
       raise(SIGINT);
    }

     raise(SIGSTOP);
}

void continueTesting(int signum){
    printf("OH BOY LOOK WHO IS BACK with rem = %d\n", remainingtime);
    remainingtime++;
    initClk();
}
