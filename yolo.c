#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int *pid;

void yes()
{
    printf("%d", *pid);
}

int main()
{
    int num = 2;
    pid = &num;
    yes();
}