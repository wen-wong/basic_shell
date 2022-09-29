#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define LENGTH 20

int MAX_USER_INPUT = 1000;
char *args[20];
int bg = 0;
int builtin_cmd = 0;
// int output_redirection = 0;

// Running the shell functions
int getcmd(char *prompt, char *args[], int *background);
int createchild(char *args[], int args_size);
int execcmd(char *command, char *args[], int args_size);
void reset_shell(char *args[], int *background, int *builtin_cmd);
// Handling the input signals
static void handle_signal(int signal);
// Built-in commands
int echo (char *args[]);
int cd (char *args[], int args_size);
int pwd(void);
int help(void);
int exit_builtin(char *args[]);
int jobs(int id);

int exec_builtin(char *args[], int cnt, int *buitlin_cmd)
{
    return execcmd(args[0], args, cnt);
}

int set_output_redireciton(char *args[], int cnt)
{
    int redirect = 0;
    for (int i = 1; i < cnt; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            close(1);
            redirect = open(args[i + 1], O_RDWR);
            break;
        }
    }
    return redirect;
}

int main(int argc, char *argv[]) 
{
    printf("%s\n", "Booting shell...");

    // Handle signal handling errors
    if (signal(SIGINT, handle_signal) == SIG_ERR)
    {
        printf("Could not bind the SIGINT signal handler\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTSTP, handle_signal) == SIG_ERR)
    {
        printf("Could not bind the SIGINT signal handler\n");
        exit(EXIT_FAILURE);
    }

    // Running the shell
    while(1) 
    {
        int cnt = getcmd("\n>> ", args, &bg);
        int output_redirection = set_output_redireciton(args, cnt);
        int builtin = exec_builtin(args, cnt, &builtin_cmd);
        if (builtin < 0) createchild(args, cnt);
        reset_shell(args, &bg, &builtin_cmd);
    }
    return 0;
}

static void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        printf("Caught signal %d, and now exiting the shell\n.", signal);
        exit(1);
    }
}

void reset_shell(char* args[], int *background, int *builtin_cmd)
{
    printf("--> Resetting arguments\n");
    free(*args);
    *background = 0;
    *builtin_cmd = 0;
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
    } 
    else *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) 
    {
        for (int token_index = 0; token_index < strlen(token); token_index++) 
        {
            // Handle non-printable characters
            if (token[token_index] <= 32) token[token_index] = '\0';
        }
        if (strlen(token) > 0) args[i++] = token;
    }
    args[i] = NULL;

    return i;
}

int createchild(char *args[], int args_size) 
{
    int status_code = 0;
    // TODO Avoid child process if built-in command is called
    if (args_size == 0) return -1;
    int pid = fork();
    // printf("%d", pid);
    if (pid == 0) {
        printf("--> Child running\n");

        // Running "echo" command
        status_code = execvp(args[0], args);
        printf("Command not found. Type 'help' for more information\n");
        if (status_code < 0) {
            printf("--> Child failed\n");
            exit(-1);
        } else {
            printf("--> Child succeeded\n");
            exit(1);
        }
    } else {
        printf("--> Parent waiting\n");
        if (bg == 0) waitpid(pid, &status_code, 0);
        printf("--> Parent done\n");
    }

    return 0;
}

int execcmd(char *command, char *args[], int args_size) 
{
    int status_code = -1;
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
    else if (strcmp(command, "exit") == 0)
    {
        status_code = exit_builtin(args);
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

int exit_builtin(char* args[]) 
{
    free(*args);
    exit(0);
    return 0;
}

int jobs(int id)
{
    
    return 0;
}