#include "userapp.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#define DURATION_SEC 8

typedef struct node {
    int pid;
    struct node * next;
} node_t;

void REGISTER(int pid, unsigned long period, unsigned long process){
    char buffer[200];
    sprintf(buffer, "echo R,%d,%lu,%lu > /proc/mp2/status", pid, period, process);
    system(buffer);
}

void YIELD(int pid){
    char buffer[60];
     sprintf(buffer, "echo Y,%d > /proc/mp2/status", pid);
     system(buffer);
}

void DEREGISTER(int pid){
    char buffer[20];
    sprintf(buffer, "echo D,%d > /proc/mp2/status", pid);
    system(buffer);
}



/* Added struct for easy interface passing */
int main(int argc, char **argv) {
  int pid = getpid();
  REGISTER(pid, period, processing_time); // via /proc/mp2/status
  list = READ_STATUS(); // via /proc/mp2/status
  if (pid not in list) {
    return 1;
  }

  t0 = clock_gettime(); // to know when the first job wakes up
  YIELD(pid); // via /proc/mp2/status

  //this is the real-time loop
  while (exists job) {
    wakeup_time = clock_gettime() - t0;
    do_job();
    process_time = clock_gettime() - wakeup_time;
    printf("wakeup: %d, process: %d\n", wakeup_time, process_time);
    YIELD(pid); // via /proc/mp2/status
  }

  DEREGISTER(pid); // via /proc/mp2/status
  return 0;
}