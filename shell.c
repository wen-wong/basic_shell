/*
 * README
 * ------
 * Shell interface that has the following features:
 *
 * General
 *   Forks a child process to run the command given as a user prompt in the terminal.
 * 
 * Built-in Commands
 *   - echo  prints all provided arguments to the terminal
 *   - cd    changes to given directory or print present working directory (if arguments are provided)
 *   - pwd   prints present working directory to the terminal
 *   - fg    brings a background process to the foreground using the job ID
 *   - jobs  lists the running background processes
 *   - exit  frees all allocated memory and exits the shell
 *
 * Signal Feature
 *   - SIGINT   terminates foreground child process and ignores other processes using CTRL+C
 *   - SIGTSTP  ignores any CTRL+Z signals
 *   - SIGCHLD  verifies any completed background processes
 *
 * Output Redirection
 *   Redirects given command to a file. If the file does not exist, 
 *   then it will be created with the given name.
 *
 * Command Pipe
 *   Send output from the first child process as an input to the second child process.
 * 
 * Written by Wen Hin Brandon Wong - 260949201
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_ARGS 20

/*
 * Struct: job_node
 * ----------------
 * holds the PID of a job and a reference to the next job
 *
 * pid: PID of a job
 * next: a reference to the next job
 *
 */
typedef struct job_node
{
    pid_t pid;
    int job_id;
    struct job_node* next;
} job_node;

pid_t *fg_pid;                      /* Reference to the foreground PID */
struct job_node *background_jobs;   /* List of background processes */

/* Prototype functions */
int getcmd(char *prompt, char *args[], int *background);
int exec_builtin(char *args[], int args_size, struct job_node **background_jobs);
int create_child(char *args[], int args_size, int *background, struct job_node **background_jobs, int *bg_counter);
void reset_prompt(char *args[], int *background, int *builtin);

void add_job(pid_t pid, struct job_node **jobs, int *bg_counter);
int find_pid(int job_id, struct job_node **jobs);
void remove_pid(int pid, struct job_node **jobs);
void remove_jobs(struct job_node **jobs);

void handle_signal(int signal);
void handle_child_signal(int signal);
int verify_redirection(char *symbol, char *args[], int args_size);
void setup_output_redirection(char *args[], char *prev_args[], int index);
void setup_command_pipe(char *first_args[], char *second_args[], char *args[], int args_size, int index);

int echo (char *args[], int args_size);
int cd (char *args[], int args_size);
int pwd ();
void custom_exit (char *args[], struct job_node **background_jobs);
int fg (char *args[], int args_size, struct job_node **background_jobs);
int jobs(struct job_node **background_jobs);

/*
 * Function: main
 * ---------------
 * runs the shell interface, and handles any signals related to the user prompt 
 * or background processes
 * 
 */
int main(void)
{
    char *args[MAX_ARGS];       /* User prompt arguments */

    background_jobs = NULL;
    int bg_flag = 0;            /* Background flag */
    int builtin_flag = -1;      /* Built-in flag */
    int bg_counter = 1;         /* Background process counter */

    printf("Booting Shell Program...\n");

    /* Initializing signal handlers */
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
    if (signal(SIGCHLD, handle_child_signal) == SIG_ERR)
    {
        printf("Could not bind the SITGTSTP signal handler.\n");
        exit(EXIT_FAILURE);

    }

    while(1)
    {
        /* Get the user prompt */
        int args_size = getcmd(">> ", args, &bg_flag);

        /* Execute build-in command */
        builtin_flag = exec_builtin(args, args_size, &background_jobs);
        /* Execute command with a child process */
        if (builtin_flag < 0) create_child(args, args_size, &bg_flag, &background_jobs, &bg_counter);

        /* Reset user prompt and flags to get the next user prompt */
        reset_prompt(args, &bg_flag, &builtin_flag);
    }
    return 0;
}

/*
 * Function: getcmd
 * ----------------
 * get the user prompt and set the arguments as an array
 *
 * prompt: string prompt to let the user type their message
 * args: array to store the user prompt into individual arguments
 * background: reference to the background flag
 *
 * returns: length of the arguments from the user prompt
 *
 */
int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);

    /* Get User Prompt */
    length = getline(&line, &linecap, stdin);
    if (length < 0) {
        exit(EXIT_FAILURE);
    }

    /* Verify the background flag */
    if ((loc = index(line, '&')) != NULL) 
    {
        *background = 1;
        *loc = ' ';
    } else *background = 0;

    char* addr = line;
    /* Set the arguments into an array */
    while ((token = strsep(&line, " \t\n")) != NULL)
    {
        for (int token_index = 0; token_index < strlen(token); token_index++)
        {
            if (token[token_index] <= 32) token[token_index] = '\0';
        }
        if (strlen(token) > 0) args[i++] = token;
    }
    args[i] = NULL;
    if (args[0] == NULL) free(addr);
    return i;
}

/*
* Function: exec_builtin
* ----------------------
* executes a built-in command if it exists.
*
* args: array of arguments
* args_size: size of the array of arguments
* background_jobs: linked list of the background processes
*
* returns: an integer if a built-in command has been called successfully
*
*/
int exec_builtin(char *args[], int args_size, struct job_node **background_jobs)
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
        custom_exit(args, background_jobs);
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

/*
 * Function: create_child
 * ----------------------
 * create a child process to execute the command while the parent waits
 * for the child's completion if it is placed on the foreground.
 * 
 * args: array of arguments
 * args_size: size of the array of arguments
 * background: reference to the background flag
 * background_jobs: linked list of the background processes
 * bg_counter: reference to the background process counter
 * 
 * returns: the status of the child process
 *
 */
int create_child(char *args[], int args_size, int *background, struct job_node **background_jobs, int *bg_counter)
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
        /* Prevent the child to terminate itself */
        if (signal(SIGINT, SIG_IGN) == SIG_ERR)
        {
            printf("Could not bind the SIGINT signal handler.\n");
            exit(EXIT_FAILURE);
        }

        /* Verify output redirection */
        int redir_index = verify_redirection(">", args, args_size);
        if (redir_index > 0)
        {
            /* Execute output redirection */
            char *cmd_args[redir_index + 1];
            setup_output_redirection(cmd_args, args, redir_index);
            status_code = execvp(cmd_args[0], cmd_args);
        }

        /* Verify command pipe */
        int pipe_index = verify_redirection("|", args, args_size);
        if (pipe_index > 0)
        {   
            /* Execute command pipe */
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
                /* First command passes its output to the pipe */
                close(1);
                dup(fd[1]);
                close(fd[0]);
                close(fd[1]);

                execvp(first_args[0], first_args);
            } else {
                waitpid(second_pid, &second_status_code, 0);

                /* Second command receives its input from the pipe */
                close(0);
                dup(fd[0]);
                close(fd[0]);
                close(fd[1]);

                execvp(second_args[0], second_args);
            }
        }

        /* Execute command */
        status_code = execvp(args[0], args);
    } 
    else {
        if (*background == 0) {
            fg_pid = &pid;
            waitpid(pid, &status_code, 0);
        } else add_job(pid, background_jobs, bg_counter);
    }

    return status_code;
}

/*
 * Function: setup_output_redirection
 * ----------------------------------
 * closes the file descriptor and points the assigned file.
 * If the file does not exist, then a new one is created with the same name.
 *
 * args: all arguments before the ">" of the user
 * prev_args: all arguments of the user
 * index: position of the ">" in the array of arguments
 *
 * returns: 0 to indicate its success 
 *
 */
void setup_output_redirection(char *args[], char *prev_args[], int index)
{
    for (int i = 0; i < index; i++) args[i] = prev_args[i];
    args[index] = NULL;
    close(1);
    creat(prev_args[index + 1], S_IRWXU);
}

/*
 * Function: setup_command_pipe
 * ----------------------------
 * separates the first and second arguments for the pipe.
 *
 * first_args: all arguments before the "|"
 * second_args: all arguments after the "|"
 * args: all arguments of the user
 * args_size: size of the array of arguments
 * index: position of the "|" in the array of arguments
 *
 */
void setup_command_pipe(char *first_args[], char *second_args[], char *args[], int args_size, int index)
{
    for (int i = 0; i < index; i++) first_args[i] = args[i];
    first_args[index] = NULL;

    for (int i = 0; i < args_size - (index + 1); i++)
    {
        second_args[i] = args[i + (index + 1)];
    }
    second_args[args_size - (index + 1)] = NULL;
}

/*
 * Function: reset_prompt
 * ----------------------
 * free allocated memory for the user prompt arguments and reset the flags.
 *
 * args: array to store the user prompt into individual arguments
 * background: reference to the background flag
 * builtin: reference to the built-in command flag
 *
 */
void reset_prompt(char *args[], int *background, int *builtin)
{
    free(*args);
    *background = 0;
    *builtin = -1;
}

/*
 * Function: handle_signal
 * -----------------------
 * handles the CTRL+C signal to terminate the foreground child process
 *
 * signal: signal of the user, e.g., CTRL+C (SIGINT)
 *
 */
void handle_signal(int signal)
{
    if (fg_pid == NULL) return;
    if (*fg_pid > 0) kill(*fg_pid, SIGTERM);
}

/*
 * Function: handle_child_signal
 * -----------------------------
 * handles the status of any background child process and
 * removes it from the list of jobs.
 *
 * signal: signal of a process
 *
 */
void handle_child_signal(int signal)
{
    int status_code = 0;
    int value = waitpid(-1, &status_code, WNOHANG);
    if (value > 0) remove_pid(value, &background_jobs);
}

/*
 * Function: verify_redirection
 * ----------------------------
 * finds the index of the given symbol, i.e., "|" and ">".
 *
 * symbol: target value where we find its index
 * args: array of all arguments of the user
 * args_size: size of the arguments of the user
 *
 * returns: index of the given symbol
 */
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

/*
 * Function: echo
 * --------------
 * prints all given arguments to the terminal.
 * 
 * args: array of all arguments of the user
 * args_size: size of the array of arguments of the user
 * 
 * returns: 0 to indicate its success
 * 
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
 * Function: cd
 * ------------
 * change directory or print present working directory if no arguments were given.
 * 
 * args: array of all arguments of the user
 * args_size: size of the array of arguments of the user
 * 
 * returns: status_code to indicate its success
 * 
 */ 
int cd (char *args[], int args_size)
{
    int status_code = -1;

    if (args_size == 1) status_code = pwd();
    else status_code = chdir(args[1]);

    return status_code;
}

/*
 * Function: pwd
 * -------------
 * print present working directory to the terminal.
 * 
 * returns: 0 to indicate its success
 * 
 */ 
int pwd()
{
    char path[100];

    if (getcwd(path, sizeof path) == NULL) return -1;
    printf("%s\n", path);

    return 0;
}

/*
 * Function: custom_exit
 * ---------------------
 * frees allocated memory and exit the shell.
 * 
 * args: array of all arguments of the user
 * background_jobs: reference to the list of jobs
 * 
 */ 
void custom_exit(char* args[], struct job_node **background_jobs)
{
    remove_jobs(background_jobs);
    free(*args);
    exit(EXIT_SUCCESS);
}

/*
 * Function: fg
 * ------------
 * places a background job to the foreground using the job id.
 * 
 * args: array of all arguments of the user
 * args_size: size of the array of arguments
 * background_jobs: reference to the list of jobs
 * 
 * returns: status of the foreground child process
 * 
 */ 
int fg(char *args[], int args_size, struct job_node **background_jobs)
{
    int status_code = 0;
    int job_id;
    pid_t pid;

    /* If user sends too many arguments */
    if (args_size < 2) {
        printf("fg: current: no such job\n");
        return status_code;
    }

    job_id = atoi(args[1]);
    /* If user sends an argument that is not a number */
    if (job_id == 0) {
        printf("fg: %s: no such job\n", args[1]);
        return status_code;
    }

    pid = find_pid(job_id, background_jobs);
    if (pid > 0) {
        fg_pid = &pid;
        status_code = waitpid(pid, &status_code, 0);
        /* If the PID of the job is not found */
        if (status_code < 1) printf("fg: %d: no such job\n", job_id);
        else remove_pid(pid, background_jobs);
    } else {
        printf("fg: %d: no such job\n", job_id);
    }

    return status_code;
}

/*
 * Function: jobs
 * --------------
 * lists the job ID and PID of each existing job
 * 
 * background_jobs: reference to the list of jobs
 * 
 * returns: 0 to indicate its success
 * 
 */ 
int jobs(struct job_node **background_jobs)
{
    struct job_node* ptr = *background_jobs;

    while (ptr != NULL)
    {
        if (!(ptr -> pid)) break;
        printf("[%d] %d\n", ptr -> job_id, ptr -> pid);
        ptr = ptr -> next;
    }
    free(ptr);

    return 0;
}

/*
 * Function: add_job
 * -----------------
 * adds new job to linked list of jobs
 * 
 * pid: PID of the new job
 * jobs: reference to the list of jobs
 * bg_counter: reference to the background counter to assign the process' job ID
 * 
 */ 
void add_job(pid_t pid, struct job_node **jobs, int *bg_counter)
{
    struct job_node *new_job = malloc(sizeof(job_node));
    new_job -> pid = pid;
    new_job -> job_id = *bg_counter;
    new_job -> next = NULL;

    if (*jobs == NULL)
    {
        *jobs = new_job;
    } else {
        struct job_node *add_job = *jobs;
        while(add_job -> next != NULL) 
        {
            add_job = add_job -> next;
        }

        add_job -> next = new_job;
    }
    *bg_counter += 1;
}

/*
 * Function: find_pid
 * ------------------
 * finds the PID of a job from the linked list of jobs given the job ID
 * 
 * job_id: job ID
 * background_jobs: reference to the list of jobs
 * 
 * returns: PID of the job
 * 
 */ 
int find_pid(int job_id, struct job_node **jobs)
{
    pid_t pid = 0;
    struct job_node* search_job = *jobs;
    int index = 1;

    while (search_job != NULL)
    {
        if (job_id == search_job -> job_id)
        {
            pid = search_job -> pid;
            break;
        }
        search_job = search_job -> next;
        index++;
    }

    return pid;
}

/*
 * Function: remove_pid
 * --------------------
 * removes a job using a PID
 * 
 * pid: PID of job
 * background_jobs: reference to the list of jobs
 * 
 * returns: 0 to indicate its success
 * 
 */ 
void remove_pid(int pid, struct job_node **jobs)
{
    struct job_node *current_job = *jobs;
    struct job_node *next_job;

    if (current_job != NULL && current_job -> pid == pid)
    {
        if (current_job -> next == NULL)
        {
            *jobs = NULL;
        } else {
            *jobs = (*jobs) -> next;
            current_job -> next = NULL;
        }
        free(current_job);
    } else {

        while (current_job -> next != NULL) 
        {
            if ((current_job -> next) -> pid == pid) break;
            current_job = current_job -> next;
        }
        next_job = current_job -> next;
        current_job -> next = current_job -> next -> next;
        free(next_job);
    }
}

/*
 * Function: remove_jobs
 * ---------------------
 * removes al jobs from the linked list of jobs
 * 
 * background_jobs: reference to the list of jobs
 * 
 * returns: 0 to indicate its success
 * 
 */ 
void remove_jobs(struct job_node **jobs)
{
    while((*jobs) != NULL)
    {
        struct job_node *removed_job = *jobs;
        *jobs = (*jobs) -> next;
        removed_job -> next = NULL;
        free(removed_job);
    }
}