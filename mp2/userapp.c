#include "userapp.h"
#include <fcntl.h>
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DURATION_SEC 8

/* Added struct for easy interface passing */
struct info_send
{
    char opcode;
    int pid;
    unsigned int duration_ms;
    unsigned int period_ms;

};

int main(int argc, char* argv[])
{

    int pid = getpid(); // Take the process id
    int fd;
    int duration=3, counter=0, times=2;
    unsigned int loop, loop2;
    unsigned int duration_ms = atoi(argv[1]);
    unsigned int period_ms   = atoi(argv[2]); 
    int registration_flag; 
    unsigned long long initial_time=0,final_time=0;
    struct timeval time_val ;

    clock_t start, end; 

    struct info_send send;
    send.pid = pid;
    send.duration_ms = duration_ms;
    send.period_ms = period_ms;

    printf("New PID: %05d Duration: %03d Period: %03d \n", pid, duration_ms, period_ms); 
    // Print process id for reference

    fd = open("/proc/mp2/status", O_RDWR );// Open the status file

    send.opcode = 'R';
    registration_flag = write(fd, &send, sizeof(struct info_send) ); 
    // Writes process id. 4 bytes because the pid is a I32 data type. 

    if (registration_flag == -1 ){

        printf("Not able to register: %d \n", pid);
        close(fd);
        return 0;
    }


    send.opcode = 'Y';
    write(fd, &send, sizeof(struct info_send));

    //printf("Hello app !\n");
    //start = clock();
    for (counter = 0; counter < times; ++counter)
    {   /* Used for debug, as suggested
        gettimeofday(&time_val,NULL);
        initial_time = time_val.tv_sec * 1000 * 1000 + time_val.tv_usec ;
        printf ("%d Waking up at %lld \n",pid,(initial_time)/1000);
        */
        for(loop = 0; loop < 10000; loop = loop + 2)
            for(loop2 = 0; loop2 < 10000; ++loop2)
                duration++;

        printf("App: %d - Job %d done! \n",pid, counter );
        
        /* Used for debug, as suggested
        gettimeofday(&time_val,NULL);
        final_time = time_val.tv_sec * 1000 * 1000 + time_val.tv_usec ;
        printf ("%d Sleeping at %lld \n",pid,(final_time)/1000);
        printf ("%d Time taken  %lld \n",pid,(final_time-initial_time)/1000);
        */

        if (counter + 1 == times){
            send.opcode = 'D';
            write(fd, &send, sizeof(struct info_send));
        }
        else{
            send.opcode = 'Y';
            write(fd, &send, sizeof(struct info_send));
        }
    }
    //end = clock();

    //float seconds = (float)(end - start) / CLOCKS_PER_SEC;
    //printf("%f \n",seconds );


    close(fd);

    //while(1);

    return 0;
}