#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define LENGTH 20

// Running the shell functions
int getcmd(char *prompt, char *args[], int *background);
int createchild(char *args[], int args_size, int *background);
int execcmd(char *command, char *args[], int args_size);
void reset_shell(char *args[], int *background, int *builtin_cmd);
// Handling the input signals
static void handle_signal(int signal);
// Built-in commands
int echo (char *args[], int args_size);
int cd (char *args[], int args_size);
int pwd(void);
void exit_builtin(char *args[]);
int fg(char *args[], int args_size);
int jobs(void);

int set_output_redirection(char *args[], int cnt)
{
    int redirect = 0;
    for (int i = 1; i < cnt; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            if (i + 1 > cnt) break;
            close(1);
            int fd = open(args[i + 1], O_RDWR);
            redirect = i;
            break;
        }
    }
    return redirect;
}

int main(int argc, char *argv[]) 
{
    char *args[20];             // User prompt arguments

    int bg_flag = 0;            // Background flag
    int builtin_cmd_flag = 0;   // Built-in command flag
    int out_redir_flag = 0;     // Output redirection flag
    int pipe_flag = 0;          // Pipe flag

    printf("%s\n", "Booting shell...");

    // Handle signal handling errors
    if (signal(SIGINT, handle_signal) == SIG_ERR)
    {
        printf("Could not bind the SIGINT signal handler\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
    {
        printf("Could not bind the SIGINT signal handler\n");
        exit(EXIT_FAILURE);
    }

    // Running the shell
    while(1) 
    {
        int cnt = getcmd("\n>> ", args, &bg_flag);
        // int output_redirection = set_output_redirection(args, cnt);
        // if (output_redirection > 0)
        // { 
        //     cnt = output_redirection;
        // }
        // int builtin = execcmd(args[0], args, cnt);
        // if (builtin < 0) 
        createchild(args, cnt, &builtin_cmd_flag);
        // reset_shell(args, &bg_flag, &builtin_cmd_flag);
    }
    return 0;
}

static void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        printf("%s %d%s\n.", "Caught signal", signal, ", and now exiting the shell");
        // int pid = getpid();
        // kill(pid, SIGSEGV);
    }
}

void reset_shell(char* args[], int *background, int *builtin_cmd)
{
    // TODO Close file if output redirection is called
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

    if (length <= 0) exit(-1);

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

int createchild(char *args[], int args_size, int *background) 
{
    int status_code = 0;
    // TODO Avoid child process if built-in command is called
    // if (args_size == 0) return -1;
    int pid = fork();
    if (pid == 0) {
        status_code = execvp(args[0], args);
        if (status_code < 0) {
            exit(-1);
        } else {
            exit(1);
        }
    } else {
        if (background == 0) waitpid(pid, &status_code, 0);
    }

    return 0;
}

int execcmd(char *command, char *args[], int args_size) 
{
    int status_code = -1;
    if (args_size <= 0) return status_code; 
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
        exit_builtin(args);
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

/*
    echo - print all arguments to the console.
*/
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
    pwd - print the current working directory
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
    exit - free any allocated memory, kills any child process and exits the shell
*/
// TODO Handle killing all child processes
void exit_builtin(char* args[]) 
{
    free(*args);
    exit(0);
}

// TODO Bring a background job to the foreground using their pid
int fg(char *args[], int args_size)
{
    if (args_size != 2) return -1;
    // ? Bring job with corresponding pid to foreground
    return 0;
}

// TODO List jobs running in the background
int jobs()
{
    return 0;
}