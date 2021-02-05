#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */
#define DELIM " \t\r\n\a"

int is_background(char **args) {
  int background = 0;
  size_t count = 0;
  while (args[count] != NULL) {
    ++count;
  }
  /* check if the command should run in the background */
  if (strcmp(args[count - 1], "&") == 0) {
    background = 1;
    args[count - 1] = NULL;
  }
  return background;
}

/* if `is_out = 1`, redirects the output from the command args1 to filename args2
  else, redirects the input from filename args2 into command args1,
  supports background processes for command args1 if followed by `&` */
int run_red(char **args1, char **args2, int is_out) {
  pid_t pid, ppid;
  int status, out, in, background;

  pid = fork();

  background = is_background(args1);

  if (background && pid > 0) {
    /* print the pid to the console */
    printf("PID: %d\n", pid);
  }

  if (pid < 0) {
    fprintf(stderr, "run_red: fork failed\n");
    return 1;
  } else if (pid == 0) {
    if (is_out) {
      out = open(args2[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      dup2(out, 1);
      close(out);
    } else {
      in = open(args2[0], O_RDONLY);
      dup2(in, 0);
      close(in);
    }
    if (in == -1 && args2[0] != NULL) {
      fprintf(stderr, "mysh: %s: no such file or directory\n", args2[0]);
      exit(EXIT_FAILURE);
    } else if (in == -1) {
      fprintf(stderr, "mysh: please provide an input source\n");
      exit(EXIT_FAILURE);
    } else if (out == -1) {
      fprintf(stderr, "mysh: please provide an output source\n");
      exit(EXIT_FAILURE);
    } else if (execvp(args1[0], args1) == -1) {
      fprintf(stderr, "mysh: %s: command not found\n", args1[0]);
      exit(EXIT_FAILURE);
    }
  } else if (!background) {
    ppid = waitpid(pid, &status, WUNTRACED);
  }
  return 1;
}

/* creates a pipe between args1 and args2 */
int launch_pipe(char **args1, char **args2) {
  int fd[2];
  pid_t pid;
  int status;
  
  if (pipe(fd) != 0) {
    fprintf(stderr, "launch_pipe: failed to create pipe.\n");
    return 1;
  }

  pid = fork();

  if (pid < 0) {
    fprintf(stderr, "launch_pipe: fork failed\n");
    return 1;
  } else if(pid == 0) {
    dup2(fd[0], 0);
    close(fd[1]);
    if (execvp(args2[0], args2) == -1) {
      fprintf(stderr, "mysh: pipe: %s: command not found\n", args2[0]);
      exit(EXIT_FAILURE);
    }
  } else {
    dup2(fd[1], 1);
    close(fd[0]);
    if (execvp(args1[0], args1) == -1) {
      fprintf(stderr, "mysh: pipe: %s: command not found\n", args1[0]);
      exit(EXIT_FAILURE);
    }
  }
  return 1;
}

/* creates a parent process for launch_pipe, supports background
  processes if second command followed by `&` */
int run_pipe(char **args1, char **args2) {
  pid_t pid, ppid;
  int status, background = 0;

  pid = fork();

  background = is_background(args2);

  if (background && pid > 0) {
    /* print the pid to the console */
    printf("PID: %d\n", pid);
  }

  if (pid < 0) {
    fprintf(stderr, "run_pipe: fork failed\n");
    return 1;
  } else if (pid == 0) {
    status = launch_pipe(args1, args2);
  } else if (!background) {
    ppid = waitpid(pid, &status, WUNTRACED);
  }
  return 1;
}

/* runs the change directory command */
int run_cd(char **args) {
  if (args[1] != NULL) {
    if (chdir(args[1]) != 0) {
      fprintf(stderr, "mysh: cd: %s: no such file or directory\n", args[1]);
      return 1;
    }
  }
  return 1;
}

/* exit the shell */
int run_exit(char **args) {
  return 0;
}

/* creates a child process to run regular linux commands
  such as ls, ps, pwd, etc., has support for the & feature
  which runs the child process in the background */
int run_command(char **args) {
  pid_t pid, ppid;
  int status, background = 0;

  /* fork a child process */
  pid = fork();
    
  background = is_background(args);

  if (background && pid > 0) {
    /* print the pid to the console */
    printf("PID: %d\n", pid);
  }

  if (pid < 0) { /* error occurred */ 
    fprintf(stderr, "run_command: fork failed\n");
    return 1;
  } else if (pid == 0) { /* child process */
    /* if to run in the background, redirect output to /dev/null */
      if (background) {
        int out = open("/dev/null", O_WRONLY);
        dup2(out, 1);
        close(out);
      }
      if (execvp(args[0], args) == -1) {
        fprintf(stderr, "mysh: %s: command not found\n", args[0]);
        exit(EXIT_FAILURE);
      }
  } else if (!background) { /* parent process */
  /* parent will wait for the child to complete */
    ppid = waitpid(pid, &status, WUNTRACED);
  }
  return 1;
}

/* execute builtin shell commands or linux commands */
int execute(char **args) {
  if (args[0] == NULL) {
    return 1;
  }

  if (strcmp(args[0], "cd") == 0) {
    return run_cd(args);
  }

  if (strcmp(args[0], "exit") == 0) {
    return run_exit(args);
  }

  return run_command(args);
}

/* splits the input into two arrays: the args before and after the pipe
  or i/o redirection operators. Assumes that only one of '|', '>', or '<'
  will be present in the input, does not support the chaining of
  these operators */
int split_other(char *input) {
  char **args = malloc((MAX_LINE/2 + 1) * sizeof(char*));
  char **args1 = malloc((MAX_LINE/2 + 1) * sizeof(char*));
  int pip = 0, red_out = 0, red_in = 0;

    if (!args || !args1) {
      fprintf(stderr, "split_other: malloc error\n");
      exit(EXIT_FAILURE);
    }

    char *token = strtok(input, DELIM);
    int i = 0, j = 0;

    while(token != NULL) {
      if (strcmp(token, "|") == 0){
        pip = 1;
        token = strtok (NULL, DELIM);
        continue;
      } else if (strcmp(token, ">") == 0) {
        red_out = 1;
        token = strtok (NULL, DELIM);
        continue;
      } else if (strcmp(token, "<") == 0) {
        red_in = 1;
        token = strtok (NULL, DELIM);
        continue;
      }

      if (pip || red_out || red_in) {
        args1[j++] = token;
      } else {
        args[i++] = token;
      }
      token = strtok (NULL, DELIM);
    }
    args[i] = NULL;
    args1[j] = NULL;

    if (pip) {
      return run_pipe(args, args1);
    } else if (red_out) {
      return run_red(args, args1, 1);
    } else if (red_in) {
      return run_red(args, args1, 0);
    }
    return 1;
}

/* splits the input into an array of arguments with whitespace
  removed, assumes that the input given is <= 80 characters */
char **split_args(char *input) {
    char **args = malloc((MAX_LINE/2 + 1) * sizeof(char*));

    if (!args) {
      fprintf(stderr, "split_args: malloc error\n");
      exit(EXIT_FAILURE);
    }

    char *token = strtok(input, DELIM);
    int i = 0;
    while (token != NULL) {
      args[i++] = token;
      token = strtok (NULL, DELIM);
    }
    args[i] = NULL;
    return args;
}

/* read the input from the command line */
char *read_input(void) {
    char *input = NULL;
    size_t size = 0;
    ssize_t chars = getline(&input, &size, stdin);

    if (chars == -1){
      if (feof(stdin)) {
        exit(EXIT_SUCCESS);
      } else {
        fprintf(stderr, "read_input: getline error\n");
        exit(EXIT_FAILURE);
      }
    }
    return input;
}

/* trim the leading and trailing whitespace of the input */
char *trim_space(char *str) {
  char *str2;
  /* leading */
  while (isspace((unsigned char)*str)) str++;

  if(*str == 0)
    return str;
  /* trailing */
  str2 = str + strlen(str) - 1;
  while (str2 > str && isspace((unsigned char)*str2)) str2--;

  str2[1] = '\0';
  return str;
}

int main(void) {
  int should_run = 1; /* flag to determine when to exit program */
  char **args; /* command line arguments */
  char *input;
  char history[MAX_LINE];
  int status = 0, has_hist = 0, pip = 0, red = 0;

  while (should_run) {
    printf("mysh:~$ ");
    input = read_input();
    strcpy(input, trim_space(input));

    /* check if the history command was entered */
    if (strcmp(input, "!!") == 0) {
      /* if no previous command has been entered */
      if (has_hist == 0) {
        printf("No commands in history.\n");
        continue;
      } else {
        /* a previous command has been logged, so copy the command 
          saved in history over the input string */
        strcpy(input, history);
        /* print the command to the console */
        printf("%s\n", input);
      }
    } else if (strcmp(input, "") != 0) {
      /* if anything more than whitespace was entered, save the
        command to history */
      has_hist = 1;
      strcpy(history, input);
    }
    
    /* check for pipes or redirection */
    int i = 0;
    while (input[i] != '\0') {
      if (input[i] == '|') {
        pip = 1;
      }
      if (input[i] == '>' || input[i] == '<') {
        red = 1;
      }
      i++;
    }
  
    /* split args and execute commands accordingly */
    if (pip || red) {
      status = split_other(input);
      pip = 0; red = 0;
    } else {
      args = split_args(input);
      status = execute(args);
    }

    should_run = status;

    free(input);
    free(args);

    fflush(stdout);

  }
  return 0;
}
