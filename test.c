#include <string.h>
#include <stdio.h>

int main(void)
{
    char *args[4];
    char *cmd[2];

    args[0] = "ls";
    args[1] = "-l";
    args[2] = ">";
    args[3] = "a.txt";

    for (int i = 0; i < 2; i++)
    {
        // char value[20];
        // strcpy(value, args[i]);
        cmd[i] = args[i];
        // printf("%s\n", cmd[i]);
    }
    for (int i = 0; i < 2; i++)
    {
        printf("%s\n", cmd[i]);
    }
    // cmd[0] = value;
    // printf("%s\n", cmd[0]);
    return 0;
}