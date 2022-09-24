#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// Libraries to call waitpid
#include <sys/types.h>
#include <sys/wait.h>

int MAX_USER_INPUT = 1000;
char *args[20];
int bg = 0;
int getcmd(char *prompt, char *args[], int *background);
int createchild(char *args[], int args_size);
int execcmd(char *command, char *args[], int args_size);
// static void signalHandler(int sig);

// command prompts
int echo (char *args[]);
int cd (char *args[], int args_size);
int pwd(void);
int help(void);

int main(int argc, char *argv[]) 
{
    printf("%s\n", "Booting shell...");
    while(1) 
    {
        bg = 0;
        int cnt = getcmd("\n>> ", args, &bg);
        // Create child process
        createchild(args, cnt);

        // Reset all arguments to accept the new command
        printf("--> Resetting arguments\n");
        memset(args, 0, cnt);
    }
    return 0;
}

int getcmd(char *prompt, char *args[], int *background) 
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;
    
    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0) 
    {
        exit(-1);
    }

    // Handle background process
    if ((loc = index(line, '&')) != NULL) 
    {
        *background = 1;
        *loc = ' ';
    } else 
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) 
    {
        for (int token_index = 0; token_index < strlen(token); token_index++) 
        {
            // Handle non-printable charactersß
            if (token[token_index] <= 32)
                token[token_index] = '\0';
        }
        if (strlen(token) > 0)
            args[i++] = token;
    }

    return i;
}

int createchild(char *args[], int args_size) 
{
    int status_code = 0;
    int pid = fork();
    // printf("%d", pid);
    if (pid == 0) {
        printf("--> Child running\n");

        // Running "echo" command
        status_code = execcmd(args[0], args, args_size);

        if (status_code < 0) {
            printf("--> Child failed\n");
            exit(-1);
        } else {
            printf("--> Child succeeded\n");
            exit(1);
        }
    } else {
        printf("--> Parent waiting\n");
        waitpid(pid, &status_code, 0);
        printf("--> Parent done\n");
    }

    return 0;
}

int execcmd(char *command, char *args[], int args_size) 
{
    int status_code = 0;
    if (strcmp(command, "echo") == 0)
    {
        status_code = echo(args);
    }
    else if (strcmp(command, "cd") == 0)
    {
        status_code = cd(args, args_size);
    }
    else if (strcmp(command, "pwd") == 0)
    {
        status_code = pwd();
    }
    else if (strcmp(command, "help") == 0)
    {
        status_code = help();
    }
    else
    {
        status_code = execvp(command, args);
        printf("Command not found. Type 'help' for more information\n");
        // status_code = -1;
    }
    return status_code;
}

/*
    echo - print all arguments to the console.
*/
int echo (char *args[]) 
{
    for (int i = 1; i < sizeof *args; i++) 
    {
        if (args[i] == NULL) continue;
        printf("%s ", args[i]);
    }
    printf("\n");

    return 0;
}

/*
    cd - changes directory based on the given path.
         If no path was provided, then it will print the current working directory.
*/
int cd (char *args[], int args_size)
{
    int status_code = 0;
    if (args_size == 1)
    {
        status_code = pwd();
    }
    else
    {
        status_code = chdir(args[1]);
    }
    return status_code;
}

/*
    pwd - print the current working directory.
*/
int pwd ()
{
    char path[100];
    if (getcwd(path, sizeof path) == NULL)
    {
        return -1;
    }
    printf("%s\n", path);
    return 0;
}

/*
    help - print the available commands.
*/
int help(void)
{
    printf("\nHere are the available commands:\n");
    printf("\becho\ttakes a string with spaces as arguments, and prints its arguments.\n");
    return 0;
}