#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#define KEY 0x1030		// key for our semaphore group, can be any number
#define PHIL_NUM 5		// same as fork number
#define EATING_TIME 1	// how long it takes for a philosopher to eat
#define THINKING_TIME 2 // for how long philosphers think before getting hungry again
#define EAT_LIMIT 100	// how many meals can a philosopher eat

short unsigned int philosopher_id, sem_id;

void grab_forks(short unsigned int left_fork_id)
{
	short unsigned int right_fork_id = (left_fork_id + 4) % 5;
 /* struct sembuf
	{
		sem_num - semaphore number in set
		sem_op - semaphore operation, how much to add/substract from semaphore
		sem_flg - operation flag, either IPC_NOWAIT or SEM_UNDO
	} */
	struct sembuf fork_semaphore[2] = {{right_fork_id, -1, 0}, {left_fork_id, -1, 0}};
	// semop(semaphore_id, sembuf, number_of_semaphores_which_we_modify) - modifies semaphores
	semop(sem_id, fork_semaphore, 2);
}

void put_away_forks(short unsigned int left_fork_id)
{
	short unsigned int right_fork_id = (left_fork_id + 4) % 5;
	struct sembuf fork_semaphore[2] = {{right_fork_id, 1, 0}, {left_fork_id, 1, 0}};
	semop(sem_id, fork_semaphore, 2);
}

void eat(int meal_number)
{
	meal_number+=1;
	grab_forks(philosopher_id); // philosopher id is the same as fork id
	printf("Philosopher %d is eating using fork %d in left hand and %d in right hand.\n", philosopher_id, philosopher_id, ((philosopher_id + 4) % 5));
	sleep(EATING_TIME);
	printf("Philosopher %d ate his %d", philosopher_id, meal_number);
	switch (meal_number)
	{
	case 1:
		printf("st");
		break;
	case 2:
		printf("nd");
		break;
	case 3:
		printf("rd");
		break;
	default:
		printf("th");
		break;
	}
	printf(" meal.\n");
	put_away_forks(philosopher_id);
}

void think()
{
	printf("Philosopher %d is thinking.\n", philosopher_id);
	sleep(THINKING_TIME);
}

void philosopher_thread()
{
	printf("Philosopher %d sits at the table.\n", philosopher_id);
	bool hungry = true;
	int i = 0;
	while(i < EAT_LIMIT)
	{
		if (hungry)
		{
			eat(i);
			hungry = false;
			i++;
		}
		else
		{
			think();
			hungry = true;
		}
	}
}

int main()
{
	pid_t pid;
	// semget(key_t key, int sem_number, int access-rights | IPC_CREAT) where:
	// key - semaphore set identifier, arbitrary number chosen by us
	// sem_number - number of semaphores we want to initialise
	// access-rights - define permissions (for owner, group, and others) for semaphore set
	// execute permissions don't matter and write permissions change whether you can edit the semaphore values
	sem_id = semget(KEY, PHIL_NUM, 0200 | IPC_CREAT);	// 0644 = rw- r-- r--
	if (sem_id < 0)
	{
		perror("semget");
		exit(10);
	}
	for (int i = 0; i < PHIL_NUM; i++)
	{
		// sets semaphores' values
		// semctl(semaphore_set_id, number_of_semaphore_in_set, command, command_parameters)
		// here command is SETVAL which is a macro that specifies that we are setting value of the semaphore (SETVAL = 16)
		if (semctl(sem_id, i, SETVAL, 1) < 0)	// if semctl == -1 that means semctl failed
		{
			perror("semctl");
			exit(11);
		}
	}
	for (int i = 0; i < PHIL_NUM; i++)
	{
		pid = fork();
		// if child
		if (pid == 0)
		{
			philosopher_id = i;
			philosopher_thread();
			printf("Philosopher %d has left the table.\n", philosopher_id);
			return 0;
		}
		else if (pid == -1)
		{
			perror("Error: couldn't fork()\n");
			exit(12);
		}
	}
	while (wait(NULL) != -1) {}	// wait for all children to finish
	// Deallocation of semaphores:
	if (semctl(sem_id, 0, IPC_RMID, 1) < 0)
	{
		perror("Error: Couldn't deallocate semaphores.\n");
		exit(13);
	}
	return 0;
}