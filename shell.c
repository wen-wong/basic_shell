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
int exec_builtin(char *args[], int args_size, int *background, int *builtin, struct job_node **head);
int create_child(char *args[], int args_size, int *background, struct job_node **head);
int reset_prompt(char *args[], int *background, int *builtin);

void handle_signal(int signal);
int verify_output_redir(char *args[], int args_size);

int echo (char *args[], int args_size);
int cd (char *args[], int args_size);
int pwd ();
void custom_exit (char *args[], int *background, int *builtin);
int fg (char *args[], int args_size, struct job_node **head);
int jobs(struct job_node **head);

void add_job(pid_t pid, struct job_node **head)
{
    struct job_node* temp = (struct job_node*) malloc(sizeof(job_node));
    temp -> pid = pid;
    if (*head == NULL)
    {
        *head = temp;
        (*head) -> next = NULL;
    } else {
        struct job_node* ptr = *head;

        while(ptr -> next != NULL) ptr = ptr -> next;

        ptr -> next = temp;
        temp -> next = NULL;
    }
}

int main(int argc, char *argv[])
{

    char *args[20];
    struct job_node *head;

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

        builtin_flag = exec_builtin(args, args_size, &bg_flag, &builtin_flag, &head);
        if (builtin_flag < 0) create_child(args, args_size, &bg_flag, &head);

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

int exec_builtin(char *args[], int args_size, int *background, int *builtin, struct job_node **head)
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
        status_code = fg(args, args_size, head);
    }
    else if (strcmp(command, "jobs") == 0)
    {
        status_code = jobs(head);
    }

    return status_code;
}

int create_child(char *args[], int args_size, int *background, struct job_node **head)
{
    int status_code = 0;

    if (args_size == 0) return status_code;

    pid_t pid = fork();
    if (pid == 0)
    {
        // Verify output redirection
        int redir = verify_output_redir(args, args_size);
        if (redir > 0)
        {
            char *cmd_args[redir + 1];
            for (int i = 0; i < redir; i++)
            {
                cmd_args[i] = args[i];
            }
            cmd_args[redir] = NULL;
            // Execute shell command to the provided file
            status_code = execvp(cmd_args[0], cmd_args);
        } 

        // Execute shell command
        status_code = execvp(args[0], args);
    } else {
        if (*background == 0) waitpid(pid, &status_code, 0);
        else add_job(pid, head);
    }

    return status_code;
}

int reset_prompt(char *args[], int *background, int *builtin)
{
    free(*args);
    *background = 0;
    *builtin = -1;

    return 0;
}

void handle_signal(int signal)
{
    kill(signal, SIGTERM);
}

int verify_output_redir(char *args[], int args_size)
{
    for (int index = 0; index < args_size; index++)
    {
        if (args[index] == NULL) break;
        if (strcmp(args[index], ">") == 0)
        {
            if (index + 1 > (args_size - 1)) return 0;
            close(1);
            creat(args[index + 1], S_IRWXU);
            return index;
        }
    }

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


// TODO Update once everything's implemented
void custom_exit(char* args[], int *background, int *builtin)
{
    reset_prompt(args, background, builtin);
    exit(0);
}

int find_pid(int job_id, struct job_node **head)
{
    pid_t pid = 0;
    struct job_node* ptr = *head;
    int index = 1;

    while (ptr != NULL)
    {
        if (job_id == index)
        {
            pid = ptr -> pid;
            break;
        }
        ptr = ptr -> next;
        index++;
    }

    return pid;
}

int remove_pid(int job_id, struct job_node **head)
{
    struct job_node *ptr = *head;
    struct job_node *prv;

    int flag = 0;
    int index = 1;

    if (job_id == index)
    {
        *head = (*head) -> next;
    } else {
        for (index; index < job_id; index++)
        {
            prv = prv -> next;
        }
        ptr = prv -> next;
        if (prv -> next -> next != NULL)
        {
            prv -> next = prv -> next -> next;
        }
        ptr -> next = NULL;
        free(ptr);
    }

    return 0;
}

// TODO Implement foreground feature
int fg(char *args[], int args_size, struct job_node **head)
{

    if (args_size < 2) return -1;

    int status_code = 0;
    int job_id = 0;
    int result = 0;

    job_id = atoi(args[1]);
    if (job_id == 0) return status_code;

    pid_t pid = find_pid(job_id, head);
    if (pid > 0) result = waitpid(pid, &status_code, WCONTINUED);
    if (result > 0) remove_pid(job_id, head);
    printf("%d\n", result);

    return status_code;
}

int jobs(struct job_node **head)
{
    struct job_node* ptr = *head;
    int index = 1;

    while (ptr != NULL)
    {
        printf("[%d] %d\n", index, ptr -> pid);
        ptr = ptr -> next;
        index++;
    }

    return 0;
}