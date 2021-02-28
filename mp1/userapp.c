#include "userapp.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	int mypid;
	char buffer[60];
	mypid = getpid();
	sprintf(buffer, "echo %d > /proc/mp1/status", mypid);
	system(buffer);
	return 0;
}
