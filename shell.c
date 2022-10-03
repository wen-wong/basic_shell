#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

typedef struct job_node
{
    pid_t pid;
    struct job_node* next;
} job_node;

int getcmd(char *prompt, char *args[], int *background);
int exec_builtin(char *args[], int args_size, int *background, int *builtin, struct job_node **background_jobs);
int create_child(char *args[], int args_size, int *background, struct job_node **background_jobs);
int reset_prompt(char *args[], int *background, int *builtin);

void add_job(pid_t pid, struct job_node **jobs);
int find_pid(int job_id, struct job_node **jobs);
int remove_pid(int job_id, struct job_node **jobs);
int remove_jobs(struct job_node **jobs);

void handle_signal(int signal);
int verify_redirection(char *symbol, char *args[], int args_size);
int setup_output_redirection(char *args[], char *prev_args[], int index);
int setup_command_pipe(char *first_args[], char *second_args[], char *args[], int args_size, int index);

int echo (char *args[], int args_size);
int cd (char *args[], int args_size);
int pwd ();
void custom_exit (char *args[], int *background, int *builtin, struct job_node **background_jobs);
int fg (char *args[], int args_size, struct job_node **background_jobs);
int jobs(struct job_node **background_jobs);

int main(int argc, char *argv[])
{

    char *args[20];
    struct job_node *background_jobs;

    int bg_flag = 0;
    int builtin_flag = -1;

    if (signal(SIGINT, handle_signal) == SIG_ERR)
    {
        printf("Could not bind the SIGINT signal handler.\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
    {
        printf("Could not bind the SITGTSTP signal handler.\n");
        exit(EXIT_FAILURE);
    }

    printf("\n");

    while(1)
    {
        int args_size = getcmd(">> ", args, &bg_flag);

        builtin_flag = exec_builtin(args, args_size, &bg_flag, &builtin_flag, &background_jobs);
        if (builtin_flag < 0) create_child(args, args_size, &bg_flag, &background_jobs);

        reset_prompt(args, &bg_flag, &builtin_flag);
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

int exec_builtin(char *args[], int args_size, int *background, int *builtin, struct job_node **background_jobs)
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
        custom_exit(args, background, builtin, background_jobs);
    }
    else if (strcmp(command, "fg") == 0)
    {
        status_code = fg(args, args_size, background_jobs);
    }
    else if (strcmp(command, "jobs") == 0)
    {
        status_code = jobs(background_jobs);
    }

    return status_code;
}

int create_child(char *args[], int args_size, int *background, struct job_node **background_jobs)
{
    int status_code = 0;
    int pipe_flag = 0;

    if (args_size == 0) return status_code;

    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Error creating child process\n");
        return status_code;
    }
    if (pid == 0)
    {
        // Verify output redirection
        int redir_index = verify_redirection(">", args, args_size);
        if (redir_index > 0)
        {
            // Call output redirection
            char *cmd_args[redir_index + 1];
            setup_output_redirection(cmd_args, args, redir_index);
            status_code = execvp(cmd_args[0], cmd_args);
        }

        // Verify command pipe
        int pipe_index = verify_redirection("|", args, args_size);
        if (pipe_index > 0)
        {   
            // Call command pipe
            pipe_flag = 1;
            
            int fd[2];
            int second_status_code = 0;
            pid_t second_pid;

            if (pipe(fd) == -1)
            {
                printf("Error creating pipe\n");
                exit(EXIT_FAILURE);
            }

            char *first_args[pipe_index + 1];
            char *second_args[(args_size - (pipe_index + 1)) + 1];

            setup_command_pipe(first_args, second_args, args, args_size, pipe_index);

            second_pid = fork();

            if (second_pid < 0)
            {
                printf("Error creating child process\n");
                exit(EXIT_FAILURE);
            }
            if (second_pid == 0)
            {
                close(1);
                dup(fd[1]);
                close(fd[0]);
                close(fd[1]);

                execvp(first_args[0], first_args);
            } else {
                waitpid(second_pid, &second_status_code, 0);

                close(0);
                dup(fd[0]);
                close(fd[0]);
                close(fd[1]);

                execvp(second_args[0], second_args);
            }
        }

        // Execute command
        status_code = execvp(args[0], args);
    } 
    else {
        if (*background == 0) {
            waitpid(pid, &status_code, 0);
        } else add_job(pid, background_jobs);
    }

    return status_code;
}

int setup_output_redirection(char *args[], char *prev_args[], int index)
{
    for (int i = 0; i < index; i++)
    {
        args[i] = prev_args[i];
    }
    args[index] = NULL;
    close(1);
    creat(prev_args[index + 1], S_IRWXU);
    return 0;
}

int setup_command_pipe(char *first_args[], char *second_args[], char *args[], int args_size, int index)
{
    for (int i = 0; i < index; i++)
    {
        first_args[i] = args[i];
    }
    first_args[index] = NULL;

    for (int i = 0; i < args_size - (index + 1); i++)
    {
        second_args[i] = args[i + (index + 1)];
    }
    second_args[args_size - (index + 1)] = NULL;

    return 0;
}

int execute_command_pipe(char *args[], int index)
{

}

// Free allocated memory for the user prompt arguments and reset the flags
int reset_prompt(char *args[], int *background, int *builtin)
{
    free(*args);
    *background = 0;
    *builtin = -1;

    return 0;
}

// Handle CTRL+C signal to kill all foreground child processes
void handle_signal(int signal)
{
    kill(signal, SIGTERM);
}

// Return the index of the symbol, e.g., used in output redirection to find ">" and command piping to find "|"
int verify_redirection(char *symbol, char *args[], int args_size)
{
    for (int index = 0; index < args_size; index++)
    {
        if (args[index] == NULL) break;
        if (strcmp(args[index], symbol) == 0)
        {
            if (index + 1 > (args_size - 1)) return 0;
            return index;
        }
    }

    return 0;
}

// Print given arguments
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

// Change directory or print present working directory if no arguments were given
int cd (char *args[], int args_size)
{
    int status_code = -1;

    if (args_size == 1) status_code = pwd();
    status_code = chdir(args[1]);

    return status_code;
}

// Print present working directory
int pwd()
{
    char path[100];

    if (getcwd(path, sizeof path) == NULL) return -1;
    printf("%s\n", path);

    return 0;
}

// Free allocated memory and exit the shell
void custom_exit(char* args[], int *background, int *builtin, struct job_node **background_jobs)
{
    reset_prompt(args, background, builtin);
    remove_jobs(background_jobs);
    exit(0);
}

// Place a background job to the foreground using the job id
int fg(char *args[], int args_size, struct job_node **background_jobs)
{
    int status_code = -1;
    int job_id;
    pid_t pid;

    if (args_size < 2) {
        printf("fg: current: no such job\n");
        return status_code;
    }

    job_id = atoi(args[1]);
    if (job_id == 0) {
        printf("fg: %s: no such job\n", args[1]);
        return status_code;
    }

    pid = find_pid(job_id, background_jobs);
    if (pid > 0) status_code = waitpid(pid, &status_code, WCONTINUED);

    if (status_code < 1) printf("fg: %d: no such job\n", job_id);
    else remove_pid(job_id, background_jobs);

    return status_code;
}

// List the job id and pid of each existing job
int jobs(struct job_node **background_jobs)
{
    struct job_node* ptr = *background_jobs;
    int index = 1;

    while (ptr != NULL)
    {
        printf("[%d] %d\n", index, ptr -> pid);
        ptr = ptr -> next;
        index++;
    }

    return 0;
}

// Add a new job to the linked list
void add_job(pid_t pid, struct job_node **jobs)
{
    struct job_node* new_job = (struct job_node*) malloc(sizeof(job_node));
    new_job -> pid = pid;
    if (*jobs == NULL)
    {
        *jobs = new_job;
        (*jobs) -> next = NULL;
    } else {
        struct job_node* current_job = *jobs;

        while(current_job -> next != NULL) current_job = current_job -> next;

        current_job -> next = new_job;
        new_job -> next = NULL;
    }
}

// Find a job based on their job id
int find_pid(int job_id, struct job_node **jobs)
{
    pid_t pid = 0;
    struct job_node* current_job = *jobs;
    int index = 1;

    while (current_job != NULL)
    {
        if (job_id == index)
        {
            pid = current_job -> pid;
            break;
        }
        current_job = current_job -> next;
        index++;
    }

    return pid;
}

// Remove a job based on their job id
int remove_pid(int job_id, struct job_node **jobs)
{
    struct job_node *current_job;
    struct job_node *prev_job = *jobs;

    if (job_id == 1)
    {
        current_job = *jobs;
        if ((*jobs) -> next != NULL) *jobs = (*jobs) -> next;
        else *jobs = NULL;
        free(current_job);
    } else {
        for (int i = 1; i < job_id - 1; i++)
        {
            prev_job = prev_job -> next;
        }
        if (prev_job -> next -> next == NULL)
        {
            prev_job -> next = NULL;
        } else {
            current_job = prev_job -> next;
            prev_job -> next = prev_job -> next -> next;
            current_job = NULL;
            free(current_job);
        }
    }

    return 0;
}

// Remove all jobs in a linked list
int remove_jobs(struct job_node **jobs)
{
    while((*jobs) != NULL)
    {
        struct job_node *current_job = *jobs;
        *jobs = (*jobs) -> next;
        current_job -> next = NULL;
        free(current_job);
    }
    free(*jobs);
    return 0;
}