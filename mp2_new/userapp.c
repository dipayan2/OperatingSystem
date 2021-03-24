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
#include <stdexcept>
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

int exists_job(int pid){
   FILE *fp;
  char path[1035];
  int val = 0;
  


    /* Open the command for reading. */
  fp = popen("cat /proc/mp2/status", "r");
    if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path), fp) != NULL) {
         printf("%s", path);
         
    }

  /* close */
  pclose(fp);

}

/* Added struct for easy interface passing */
int main(int argc, char **argv) {
  int pid = getpid();
  struct timespec start, stop;
   clockid_t clk_id;
   clk_id = CLOCK_REALTIME;
  long period = 1400;
 long processing_time = 300;
  REGISTER(pid, period, processing_time); // via /proc/mp2/status
  list = READ_STATUS(); // via /proc/mp2/status
  if (pid not in list) {
    return 1;
  }

  clock_gettime(clk_id, &start); // to know when the first job wakes up
  YIELD(pid); // via /proc/mp2/status

  //this is the real-time loop
  while (exits_job() {
    wakeup_time = clock_gettime() - t0;
    do_job();
    process_time = clock_gettime() - wakeup_time;
    printf("wakeup: %d, process: %d\n", wakeup_time, process_time);
    YIELD(pid); // via /proc/mp2/status
  }

  DEREGISTER(pid); // via /proc/mp2/status
  return 0;
}