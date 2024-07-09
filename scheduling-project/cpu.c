#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>

#define ROW 100
#define COL ROW

// these variables are declared globally to use signal handler
int num = 0; // number of processes
int dur = 0; // duration time
int count = 0; // calc count
struct timespec totalbegin; // time structure to calc entire time
double totaltime = 0; // get entire time
int cpuid = 0; // id of process (differ to pid)

struct sched_attr { // structure that required by setattr
	uint32_t size;
	uint32_t sched_policy; // SCHED_RR
	uint64_t sched_flags;
	int32_t sched_nice;
	uint32_t sched_priority; // 10

	uint64_t sched_runtime;
	uint64_t sched_deadline;
	uint64_t sched_period;
};

void handler(int sig) { // signal handler
	if (sig == SIGINT) { // if sig get signal interrupt (ctrl + c)
		printf("DONE!! PROCESS #%02d : %06d %ld\n", cpuid, count, (long int)(totaltime)); // print count until sigint
		exit(0); // end process
	}
}

		
int calc(int cpuid, int dur) { // calculate matrix
	int matrixA[ROW][COL];
	int matrixB[ROW][COL];
	int matrixC[ROW][COL];
	int i,j,k;

	struct timespec begin, end; // declare time structure

	clock_gettime(CLOCK_MONOTONIC, &totalbegin); // begin clock that calc entire time
	clock_gettime(CLOCK_MONOTONIC, &begin); // begin clock that calc 100ms

	while(1){
		signal(SIGINT, handler); // using signal handler
		for (i = 0;i < ROW; i++){
			for (j = 0;j<COL;j++){
				for (k = 0;k < COL;k++){
					matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
				}
			}
		}
		count++; // count incrementation
		
		clock_gettime(CLOCK_MONOTONIC, &end); // end clock

		long long t = (end.tv_sec - begin.tv_sec) * 1000 + (end.tv_nsec - begin.tv_nsec) / 1000000; // get time from begin to end
		if (t >= 100) { // if t is bigger than 100ms
			printf("PROCESS #%02d count = %02d 100\n", cpuid, count);
			clock_gettime(CLOCK_MONOTONIC, &begin); // reset clock that calc 100ms
		}

		totaltime = (end.tv_sec - totalbegin.tv_sec) * 1000 + (end.tv_nsec - totalbegin.tv_nsec) / 1000000; // get time from totalbegin to end
		if (totaltime >= dur * 1000) { // if totaltime is bigger than duration
			printf("DONE!! PROCESS #%02d : %06d %d\n", cpuid, count, dur * 1000);
			break; // end loop
		}
	}
	
	return count;
}

static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) {
	return syscall(SYS_sched_setattr, pid, attr, flags);
} // sched_setattr system call

int main(int argc, char* argv[]){
	num = atoi(argv[1]); // first argument of process input
	dur = atoi(argv[2]); // second argument of process input
	
	int sched; // return value of call sched_setattr

	int pidarr[987654]; // array to store child pid (need waitpid())

	struct sched_attr attr; // variable for sched_setattr
	memset(&attr, 0, sizeof(attr)); // initialize attr
	attr.size = sizeof(struct sched_attr); // size of attr
	attr.sched_priority = 10; // priority
	attr.sched_policy = SCHED_RR; // Policy : Round Robin

	for (int i = 0; i < num; i++){
		sched = sched_setattr(getpid(), &attr, 0); // call sched_setattr
		if (sched == -1) { // if setattr fail
			printf("setattr error.\n");
			continue;
		}
		pidarr[i] = fork(); // fork child process
		if (pidarr[i] == 0) { // if child process
			cpuid = i;
			printf("Creating Process: #%02d\n", i);
			int count = calc(i, dur);
			exit(0); // prevent child to fork another child
		}
	}
	for (int i = 0; i < num; i++) {
		pid_t child_pid = waitpid(pidarr[i], NULL, 0); // wait termination of child process
	}

	return 0;
}
