#include <unistd.h>
#include <stdio.h>

#ifndef WAIT_TIME
    #define WAIT_TIME 10
#endif

int main()
{
    printf("Pausing %d\n",getpid());
    sleep(WAIT_TIME);
    printf("Resume\n");
    return 0;
}