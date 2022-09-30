#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

void handle_signal(int signal);
int getcmd(char *prompt, char *args[], int *background);
int exec_builtin(char *args[], int args_size, int *background, int *builtin);
int create_child(char *args[], int args_size, int *background);
int reset_prompt(char *args[], int *background, int *builtin);

int echo (char *args[], int args_size);
int cd (char *args[], int args_size);
int pwd ();
void custom_exit (char *args[], int *background, int *builtin);
int fg (char *args[], int args_size);
int jobs();

int main(int argc, char *argv[])
{

    char *args[20];

    int bg_flag = 0;
    int builtin_flag = -1;

    if (signal(SIGINT, handle_signal) == SIG_ERR)
    {
        printf("Could not bind the SIGNINT signal handler.\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
    {
        printf("Could not bind the SITGTSTP signal handler.\n");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        int args_size = getcmd("\n>> ", args, &bg_flag);
        builtin_flag = exec_builtin(args, args_size, &bg_flag, &builtin_flag);
        if (builtin_flag < 0) create_child(args, args_size, &bg_flag);
        reset_prompt(args, &bg_flag, &builtin_flag);
    }
    return 0;
}

// TODO Handle child process termination
void handle_signal(int signal)
{
    kill(0, 0);
}

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);

    // Get User Prompt
    length = getline(&line, &linecap, stdin);
    if (length < 0) exit(EXIT_FAILURE);

    // Verify the background flag
    if ((loc = index(line, '&')) != NULL) 
    {
        *background = 1;
        *loc = ' ';
    } else *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL)
    {
        for (int token_index = 0; token_index < strlen(token); token_index++)
        {
            if (token[token_index] <= 32) token[token_index] = '\0';
        }
        if (strlen(token) > 0) args[i++] = token;
    }
    args[i] = NULL;

    return i;
}

int exec_builtin(char *args[], int args_size, int *background, int *builtin)
{
    int status_code = -1;

    if (args_size == 0) return status_code;

    char* command = args[0];
    if (strcmp(command, "echo") == 0)
    {
        status_code = echo(args, args_size);
    } 
    else if (strcmp(command, "cd") == 0)
    {
        status_code = cd(args, args_size);
    }
    else if (strcmp(command, "pwd") == 0)
    {
        status_code = pwd();
    }
    else if (strcmp(command, "exit") == 0)
    {
        custom_exit(args, background, builtin);
    }
    else if (strcmp(command, "fg") == 0)
    {
        status_code = fg(args, args_size);
    }
    else if (strcmp(command, "jobs") == 0)
    {
        status_code = jobs();
    }

    return status_code;
}

int verify_output_redir(char *args[], int args_size)
{
    for (int index = 0; index < args_size; index++)
    {
        if (strcmp(args[index], ">") == 0)
        {
            if (index + 1 > (args_size - 1)) return -1;
            close(1);
            creat(args[index + 1], O_RDWR);
            // if (access(args[index + 1], F_OK) != 0) printf("Files does not exists\n");
            // open(args[index + 1], O_RDWR);
            return 0;
        }
    }

    return -1;
}

int create_child(char *args[], int args_size, int *background)
{
    int status_code = 0;
    
    pid_t pid = fork();
    if (pid == 0)
    {
        // Verify output redirection
        int redir = verify_output_redir(args, args_size);
        printf("Redirection: %d\n", redir);
        // Verify command piping


        status_code = execvp(args[0], args);

        if (status_code < 0) exit(EXIT_FAILURE);
        exit(EXIT_SUCCESS);
    } else {
        if (*background == 0) waitpid(pid, &status_code, 0);
    }

    return 0;
}

int reset_prompt(char *args[], int *background, int *builtin)
{
    free(*args);
    *background = 0;
    *builtin = -1;

    return 0;
}

int echo (char *args[], int args_size)
{
    for (int i = 1; i < args_size; i++)
    {
        if (args[i] == NULL) continue;
        printf("%s ", args[i]);
    }
    printf("\n");

    return 0;
}

int cd (char *args[], int args_size)
{
    int status_code = -1;

    if (args_size == 1) status_code = pwd();
    status_code = chdir(args[1]);

    return status_code;
}

int pwd()
{
    char path[100];

    if (getcwd(path, sizeof path) == NULL) return -1;
    printf("%s\n", path);

    return 0;
}

void custom_exit(char* args[], int *background, int *builtin)
{
    reset_prompt(args, background, builtin);
    exit(0);
}

// TODO Implement foreground feature
int fg(char *args[], int args_size)
{
    return -1;
}

// TODO Implement jobs feature
int jobs()
{
    return -1;
}