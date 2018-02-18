#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokenizer.h"
#include "int_handler.h"

/* Maximun number of character of a path */
#define MAX_PATH 1024

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_exec(struct tokens *tokens);
int cmd_foo(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "print name of current/working directory"},
  {cmd_cd, "cd", "change working directory"},
  {cmd_exec, "exec", "replace current process with another program"},
  {cmd_foo, "foo", "run test code"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

/* Prints the current working directory */
int cmd_pwd(unused struct tokens *tokens) {
  char buf[MAX_PATH];
  if (getcwd(buf, MAX_PATH)) {
    printf("%s\n", buf);
    return 0;
  } else {
    printf("Unexpected error\n");
    return -1;
  }
}

/* Change current working directory */
int cmd_cd(struct tokens *tokens) {
  if (tokens_get_length(tokens) != 2) {
    printf("Must pass in only 1 argument\n");
    return -1;
  } 

  char* path = tokens_get_token(tokens, 1);
  if (chdir(path) < 0) {
    printf("Unable to cd to path: %s\n", path);
    return -1;
  }
  return 0;
}

/* Check if file exist */
bool file_exists(char* name) {
  return access(name, F_OK) == 0;

}

/* Copy from to to until reach until */
int strcpyuntil(char* to, const char* from, char until, int startpos) {
  int i = startpos;
  int j = 0;
  while (from[i] != until && from[i] != '\0') {
    to[j] = from[i];
    ++i;
    ++j;
  }
  to[j] = '\0';
  return i;
}

/* Change filename to correct path to file, return true if success 
 * Please make sure filename is big enough to hold the path */
bool find_file_from_PATH(char* filename) {
  if (file_exists(filename)) {
    return true;
  }

  const char* env = getenv("PATH");
  char path[MAX_PATH];
  int pos = -1;
  while (env[pos] != '\0') {
    pos = strcpyuntil(path, env, ':', pos + 1);
    strcat(path, "/");
    strcat(path, filename);
    if (file_exists(path)) {
      strcpy(filename, path);
      return true;
    }
  }
  return false;
}

/* Replace current process by executing argv */
int exec(int argc, char** argv) {
  char* filename = argv[0];
  if (!find_file_from_PATH(filename)) {
    printf("Unable to find %s\n", filename);
    return -1;
  }

  // Handle io redirect
  if (argc > 2) {
    // Reirect output if “[process] > [file]”
    if (strcmp(argv[argc - 2], ">") == 0) {
      int fout = open(argv[argc - 1], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      if (dup2(fout, STDOUT_FILENO) < 0) {
        printf("redir failed");
      };
      close(fout);
      argv[argc - 2] = '\0';
    } else if (strcmp(argv[argc - 2], "<") == 0) {  
      // Redirect input if “[process] < [file]”
      int fin = open(argv[argc - 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      if (dup2(fin, STDIN_FILENO) < 0) {
        printf("redir failed");
      };
      close(fin);
      argv[argc - 2] = '\0';
    }
  }

  return execv(filename, argv);
}

/* replace the shell with command called by arguments */
int cmd_exec(struct tokens *tokens) {
  int size = tokens_get_length(tokens);
  char** argv = malloc(sizeof(char*) * size); // null terminate c_str array
  for (int i = 1; i < size; ++i) {
    argv[i - 1] = tokens_get_token(tokens, i);
  }
  argv[size - 1] = NULL;
  int rtn_val = exec(size - 1, argv);
  free(argv);
  return rtn_val;
}

int cmd_foo(unused struct tokens *tokens) {
  printf("%s", getenv("PATH"));
  return 0;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }

  /* Set interrupt handler */
  signal(SIGTTOU, SIG_IGN);
}

int main(unused int argc, unused char *argv[]) {
  printf("%s", argv[0]);
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      pid_t cpid = fork();
      int status = 0;
      if (cpid > 0) {
        setpgid(cpid, cpid); // Make child process its own process group
        tcsetpgrp(STDIN_FILENO, cpid); // Set forground to child process
        
        wait(&status);
        if(status) {
          printf("Calling failed, return code: %i\n", status);
        }
        tcsetpgrp(STDIN_FILENO, shell_pgid); // Set forground back to shell
      } else if (cpid == 0) {
        /* Set interrupt handler */
        signal(SIGINT, int_handler);

        // Construct argv
        int size = tokens_get_length(tokens);
        char** argv = malloc(sizeof(char*) * (size + 1)); // null terminate c_str array
        for (int i = 0; i < size; ++i) {
          argv[i] = tokens_get_token(tokens, i);
        }
        argv[size] = NULL;
        int rtn_val = exec(size, argv);
        free(argv);
        exit(rtn_val);
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
