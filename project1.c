#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE 80 /* The maximum length command */
#define DELIM " \t\r\n\a"

int run_cd(char **args) {
  if (args[1] != NULL) {
    if (chdir(args[1]) != 0) {
      fprintf(stderr, "mysh: cd: %s: no such file or directory\n", args[1]);
    }
  }
  return 1;
}

int run_exit(char **args) {
  return 0;
}

int run(char **args) {
    pid_t pid, ppid;
    int status;
    int background = 0;

    size_t count = 0;
    while (args[count] != NULL) {
        ++count;
    }
    /* fork a child process */
    pid = fork();
    
    if(strcmp(args[count - 1], "&") == 0) {
        background = 1;
        args[count - 1] = NULL;
        if (pid > 0) {
        printf("PID: %d\n", pid);
        }
    }

    if (pid < 0) { /* error occurred */ 
        fprintf(stderr, "Fork Failed\n");
        return 1;
    } else if (pid == 0) { /* child process */
        if (execvp(args[0], args) == -1) {
          fprintf(stderr, "mysh: %s: command not found\n", args[0]);
          exit(EXIT_FAILURE);
        }
    } else if(!background){ /* parent process */
    /* parent will wait for the child to complete */
      ppid = waitpid(pid, &status, WUNTRACED);
    }
    return 1;
}

int execute(char **args) {
  if (args[0] == NULL) {
    return 1;
  }

  if (strcmp(args[0], "cd") == 0) {
    return run_cd(args);
  }

  if(strcmp(args[0], "exit") == 0) {
    return run_exit(args);
  }

  return run(args);
}

char **split_args(char *input) {
    char **args = malloc((MAX_LINE/2 + 1) * sizeof(char*));

    if (!args) {
      fprintf(stderr, "split_args: malloc error\n");
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

char *read_input(void) {
    char *input = NULL;
    size_t size = 0;
    ssize_t chars = getline(&input, &size, stdin);

    if (chars == -1){
        if (feof(stdin)) {
          exit(EXIT_SUCCESS);
        } else  {
          fprintf(stderr, "read_input: getline error\n");
          exit(EXIT_FAILURE);
        }
    }
    return input;
}

int main(void) {
  int should_run = 1; /* flag to determine when to exit program */
  char **args; /* command line arguments */
  char *input;
  int status;
  char history[MAX_LINE];
  int has_hist = 0;

  while (should_run) {
    printf("mysh:~$ ");
    input = read_input();

    if(strcmp(input, "!!\n") == 0){
      if (has_hist == 0) {
        printf("No commands in history.\n");
        continue;
      } else {
          strcpy(input, history);
          printf("%s", input);
      }
    } else {
      has_hist = 1;
      strcpy(history, input);
    }

    args = split_args(input);
    status = execute(args);

    should_run = status;

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
