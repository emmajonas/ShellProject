#include <stdio.h> 
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#define MAX_LINE 80 /* The maximum length command */
#define DELIM " \t\r\n\a"

int run_cd(char **args) {
    if (args[1] == NULL) {
        return 0;
    } else {
        if(chdir(args[1]) != 0) {
            fprintf(stderr, "mysh: cd: %s: no such file or directory\n", args[1]);
        }
    }
    exit(EXIT_SUCCESS);
}

int run(char **args) {
    pid_t pid, ppid;
    int status;
    int background = 0;

    size_t count = 0;
    while (args[count] != NULL) {
        ++count;
    }
    if(strcmp(args[count - 1], "&") == 0) {
        background = 1;
        args[count - 1] = NULL;
    }
    /* fork a child process */
    pid = fork();
    if (pid < 0) { /* error occurred */ 
        fprintf(stderr, "Fork Failed\n");
        return 1;
     } else if (pid == 0) { /* child process */
        if(strcmp(args[0], "cd") == 0) {
             run_cd(args);
        } else {
            if (execvp(args[0], args) == -1) {
                fprintf(stderr, "mysh: %s: command not found\n", args[0]);
                exit(EXIT_FAILURE);
            }
        }
    } else if(!background){ /* parent process */
    /* parent will wait for the child to complete */
        waitpid(pid, &status, 0);
        printf("Child Complete\n");
    }
}

int execute(char **args) {
    if (args[0] == NULL) {
        return 1;
    } else {
        run(args);
    }
}

char **split_line(char *input) {
    char **args = malloc((MAX_LINE/2 + 1) * sizeof(char*));

    if (!args) {
        fprintf(stderr, "split_line: allocation error\n");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(input, DELIM);
    int i = 0;
    while(token != NULL) {
        args[i++] = token;
        token = strtok (NULL, DELIM);
    }
    args[i] = NULL;
    return args;
}

char* read_line(void) {
    char *line = NULL;
    ssize_t size = MAX_LINE;

    size_t chars = getline(&line, &size, stdin);

    if (chars == -1){
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else  {
            fprintf(stderr, "read_line: error\n");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

int main(void) {
    int should_run = 1; /* flag to determine when to exit program */
    char **args; /* command line arguments */
    char *input;
    int status;
    char *history;
    int has_hist = 0;
    
    while (should_run){ 
        printf("mysh:~$ ");
        input = read_line();
        input[strlen(input)-1] = '\0';

        if(strcmp(input, "exit") == 0){
            should_run = 0;
            continue;
        } else if(strcmp(input, "!!") == 0){
            if (has_hist == 0) {
                printf("No commands in history\n");
                continue;
            } else {
                strcpy(input, history);
                printf("%s\n", input);
            }
        } else {
            has_hist = 1;
            strcpy(history, input);
        }

        args = split_line(input);
        status = execute(args);

        free(input);
        free(args);

        fflush(stdout);
        
        /** After reading user input, the steps are:  
         * (1) fork a child process using fork()  
         * (2) the child process will invoke execvp()  
         * (3) parent will invoke wait() unless command included & 
         */
    }

    return 0;
}
