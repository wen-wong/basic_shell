#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int MAX_USER_INPUT = 1000;
char *args[20];
int bg = 0;
int getcmd(char *prompt, char *args[], int *background);
// static void signalHandler(int sig);

// command prompts
int echo (char *value);

int main(int argc, char *argv[]) {
    printf("%s\n", "Booting shell...");
    while(1) {
        bg = 0;
        int cnt = getcmd("\n>> ", args, &bg);

        // Print all arguments created by getcmd
        for (int i = 0; i < 20; i++) {
            if (args[i] == NULL) continue;
            printf("%s ", args[i]);
        }

        memset(args, 0, sizeof args);
    }
    return 0;
}

int getcmd(char *prompt, char *args[], int *background) {
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;
    
    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0) {
        exit(-1);
    }

    // Handle background process
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else 
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int token_index = 0; token_index < strlen(token); token_index++) {
            // Handle non-printable charactersß
            if (token[token_index] <= 32)
                token[token_index] = '\0';
        }
        if (strlen(token) > 0)
            args[i++] = token;
    }

    return i;
}

int echo (char *value) {
    printf("%s", value);
    return 0;
}