#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

unsigned long fib(int val){
	if (val == 0){
		return 1;
	}
	else if (val == 1){
		return 1;
	}
	return fib(val-1)+fib(val-2);
}


int main(int argc, char* argv[])
{
	int mypid;
	char buffer[60];
	mypid = getpid();
	sprintf(buffer, "echo %d > /proc/mp1/status", mypid);
	system(buffer);
	int num = 20;
	if (argc > 1){
		num = atoi(argv[1]);
	}
	int i =0;

	for(i=0;i<10000;i++){
		unsigned long v;
		v = fib(num);		
	}

	return 0;
}
