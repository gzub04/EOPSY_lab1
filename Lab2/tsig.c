#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define NUM_CHILD 5
#define WITH_SIGNAL // comment out this line to launch program in mode 2, otherwise leave uncommented

#ifdef WITH_SIGNAL
volatile bool interrupt = false;

void key_interrupt(int sig)
{
    printf("\nparent[%d]: Key interrupt detected\n", getpid());
    interrupt = true;
}

void terminate_child(int sig)
{
    printf("child[%d]: child terminated\n", getpid());
    exit(1);
}
#endif

int main()
{
    pid_t children[NUM_CHILD]; // all children are stored here
    //int status;                // for waitpid

    struct sigaction sig_act;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = 0;

    for (int i = 0; i < NUM_CHILD; i++) // loop creating children
    {
        pid_t c_pid = fork();
        
    // if child
        if (c_pid == 0)
        {
            #ifdef WITH_SIGNAL
                sig_act.sa_handler = SIG_IGN;       // ignore SIGINT
                sigaction(SIGINT, &sig_act, NULL);     //
                sig_act.sa_handler = terminate_child;   // set SIGTERM to our own function
                sigaction(SIGTERM, &sig_act, NULL);        //
            #endif
            printf("parent[%d]: created child[%d]\n", getppid(), getpid());
            sleep(10);
            printf("child[%d]: execution completed\n", getpid());
            exit(0);
        }
    // if not child
        else if (c_pid == -1)
        {
            printf("parent[%d]: Failed creating a child\n", getppid());
            for ( int j=0; j <= i; j++) // terminate all child processes
            {
                if (kill(children[j], SIGTERM) == 0)
                {
                    printf("parent[%d]: sending SIGTERM signal\n", getpid());
                    printf("child[%d]: received SIGTERM signal, terminating\n", children[j]);
                }
            }
            exit(1);
        }
        #ifdef WITH_SIGNAL  // set in parent ignoring all signals except SIGCHLD and SIGINT        
            sig_act.sa_handler = SIG_IGN;   // ignore signal
            for (int i = 0; i < _NSIG; i++) // _NSIG - number of signals
            {
                sigaction(i, &sig_act, NULL);  // ignore all signals
            }
            sig_act.sa_handler = SIG_DFL;       // set SIGCHLD back to default
            sigaction(SIGCHLD, &sig_act, NULL);    //
            sig_act.sa_handler = key_interrupt;     // set SIGINT to our own version
            sigaction(SIGINT, &sig_act, NULL);         //
        #endif
        children[i] = c_pid;
        sleep(1);
        #ifdef WITH_SIGNAL
        if (interrupt)
        {
            printf("Child creation process interrupted\n");
            for ( int j=0; j <= i; j++)
            {
                kill(children[j], SIGTERM);
            }
            break;
        }
        #endif
    }
    if (interrupt == false)
        printf("All processes have been created successfully\n");

    // count children
    pid_t wait_val = wait(NULL);
    int sum_of_children_completed = 0;
    while (wait_val != -1)
    {
        sum_of_children_completed += 1;
        wait_val = wait(NULL);
    }
    printf("Number of received exit codes: %d\n", sum_of_children_completed);

    #ifdef WITH_SIGNAL
        sig_act.sa_handler = SIG_DFL;
        for (int i = 0; i < _NSIG; i++)
        {
            sigaction(i, &sig_act, NULL);
        }
    #endif
    return 0;
}