/* Read the additional functions from util.h. They may be beneficial to you
in the future */
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

/* Global variables */
/* The array for holding shell paths. Can be edited by the functions in util.c*/
char shell_paths[MAX_ENTRIES_IN_SHELLPATH][MAX_CHARS_PER_CMDLINE];
static char prompt[] = "utcsh> "; /* Command line prompt */
static char *default_shell_path[2] = {"/bin", NULL};
/* End Global Variables */

/* Convenience struct for describing a command. Modify this struct as you see
 * fit--add extra members to help you write your code. */
struct Command
{
  char **args;      /* Argument array for the command */
  char *outputFile; /* Redirect target for file (NULL means no redirect) */
};

/* Here are the functions we recommend you implement */

char **tokenize_command_line(char *cmdline);
struct Command parse_command(char **tokens);
void eval(struct Command *cmd);
int try_exec_builtin(struct Command *cmd);
void exec_external_cmd(struct Command *cmd);

/* Main REPL: read, evaluate, and print.
 * @argc: The number of arguments passed to the shell
 * @argv: The array of arguments passed to the shell
 * Return value: 0 if the shell should exit. Loops otherwise.
 */
int main(int argc, char **argv)
{

  set_shell_path(default_shell_path);

  char *line = NULL;

  if (argv[1] != NULL)
  {

    if (argv[2] != NULL)
    {

      fprintf(stderr, "An error has occurred\n");

      exit(1);
    }

    // all scripts
    struct Command file_command;
    char *args[2] = {argv[1], NULL};
    file_command.args = args;
    exec_file(&file_command);
  }
  else
  {
    while (1)
    {

      char *line = *argv;

      printf("%s", prompt);

      int length = strlen(line);

      int num_bytes = getline(&line, &length, stdin);

      if (num_bytes == -1)
      {
        fprintf(stderr, "An error has occurred\n");
        exit(1);
      }
      if (line[strlen(line) - 1] == '\n')
      {
        line[strlen(line) - 1] = '\0';
      }
      int stat = exec_line(line);

      if (stat == 1)
      {
        return 0;
      }
    }
  }

  return 0;
}

int exec_line(char *line)
{
  bool is_empty = true;
  int num_bytes = strlen(line);
  char *temp_line = line;
  while (*temp_line != '\0')
  {

    if (!isspace(*temp_line))
    {
      is_empty = false;
    }
    temp_line++;
  }

  if (is_empty)
  {
    return 0;
  }

  if (num_bytes == 1)
  {
    return 0;
  }

  if (strcmp(line, "exit") == 0)
  {

    return 1;
  }

  char *result = strchr(line, '&');

  // non-concurrent code
  if (result == NULL)
  {

    char **tokens = tokenize_command_line(line);

    struct Command token_command = parse_command(tokens);

    eval(&token_command);
    free(tokens);

    memset(line, 0, num_bytes);
    return 0;
  }

  else
  {
    // where stuff happens

    int command_length = 100;
    int num_phrases = 0;
    char **phrases = malloc(command_length * sizeof(char *));
    if (phrases == NULL)
    {
      fprintf(stderr, "An error has occurred\n");
      free(phrases);

      exit(1);
    }
    char *phrase = strtok(line, "&");

    while (phrase != NULL)
    {

      if (num_phrases >= command_length)
      {
        command_length *= 2;
        phrases = realloc(phrases, command_length * sizeof(char *));
        if (phrases == NULL)
        {
          exit(0);
        }
      }

      phrases[num_phrases] = phrase;
      num_phrases++;
      phrase = strtok(NULL, "&");
    }

    phrases[num_phrases] = NULL;

    // have our phrases -- now we need to modify parameter memory and fork on each one of them
    pid_t pid;
    for (int i = 0; phrases[i] != NULL; ++i)
    {
      char *phrase = phrases[i];
      int phrase_length = strlen(phrase);
      if (phrase[phrase_length - 1] == '\n')
      {
        phrase[phrase_length - 1] = '\0';
      }
      if (phrase[0] == ' ')
      {
        phrase = phrase + 1;
      }
      phrases[i] = phrase;
      line = phrase;
      pid = fork();
      if (pid < 0)
      {
        // Fork failed
        fprintf(stderr, "An error has occurred\n");
        free(phrases);

        exit(EXIT_FAILURE);
      }

      if (pid == 0)
      {
        break;
      }
    }
    if (pid != 0)
    {
      int res1;

      for (int i = 0; phrases[i] != NULL; ++i)
      {
        waitpid(-1, &res1, 0);
        int status1 = WEXITSTATUS(res1);

        if (status1 == 0)
        {
          free(phrases);

          return 0;
        }
      }
    }

    if (pid == 0)
    {

      // if in a child we do that otherwise nothing
      char **tokens = tokenize_command_line(line);

      if (tokens == NULL)
      {
        free(phrases);

        return 0;
      }

      struct Command token_command = parse_command(tokens);

      eval(&token_command);

      free(tokens);

      // Print (optional)

      // Clear the buffer for the next iteration
      memset(line, 0, num_bytes);
      free(phrases);

      exit(1);
    }
    free(phrases);

    return 0;
  }
}

/* NOTE: In the skeleton code, all function bodies below this line are dummy
implementations made to avoid warnings. You should delete them and replace them
with your own implementation. */

/** Turn a command line into tokens with strtok
 *
 * This function turns a command line into an array of arguments, making it
 * much easier to process. First, you should figure out how many arguments you
 * have, then allocate a char** of sufficient size and fill it using strtok()
 */
char **tokenize_command_line(char *cmdline)
{
  if (*cmdline == NULL)
  {
    return NULL;
  }

  int num_bytes = strlen(cmdline);
  if (cmdline[num_bytes - 1] == '\n')
  {
    cmdline[num_bytes - 1] = '\0';
  }

  int command_length = 100;
  int num_tokens = 0;
  char **tokens = malloc(command_length * sizeof(char *));
  if (tokens == NULL)
  {
    fprintf(stderr, "An error has occurred\n");
    exit(1);
  }
  char delimiters[] = "  \t\n";
  char *token = strtok(cmdline, delimiters);

  while (token != NULL)
  {

    if (num_tokens >= command_length)
    {
      command_length *= 2;
      tokens = realloc(tokens, command_length * sizeof(char *));
      if (tokens == NULL)
      {
        exit(1);
      }
    }

    tokens[num_tokens] = token;

    num_tokens++;
    token = strtok(NULL, delimiters);
  }

  tokens[num_tokens] = NULL;

  return tokens;
}

/** Turn tokens into a command.
 *
 * The `struct Command` represents a command to execute. This is the preferred
 * format for storing information about a command, though you are free to
 * change it. This function takes a sequence of tokens and turns them into
 * a struct Command.
 */
struct Command parse_command(char **tokens)
{
  struct Command dummy;
  dummy.args = tokens;
  dummy.outputFile = NULL;
  bool is_empty = true;
  // Let's assume that '>' indicates output redirection and appears only once in the command
  for (int i = 0; tokens[i] != NULL; ++i)
  {

    if (strcmp(tokens[i], ">") == 0)
    {
      if (!is_empty || i == 0)
      {
        fprintf(stderr, "An error has occurred\n");

        exit(0);
      }
      if (tokens[i + 1] == NULL)
      {
        fprintf(stderr, "An error has occurred\n");

        exit(0);
      }
      dummy.outputFile = tokens[i + 1]; // The file to redirect output to

      tokens[i] = NULL; // Nullify the '>' token so that args becomes a proper null-terminated array for execvp
      is_empty = false;
      if (tokens[i + 2] != NULL)
      {
        fprintf(stderr, "An error has occurred\n");

        exit(0);
      }
    }
  }
  // printf("\n");
  return dummy;
}

/** Evaluate a single command
 *
 * Both built-ins and external commands can be passed to this function--it
 * should work out what the correct type is and take the appropriate action.
 */
void eval(struct Command *cmd)
{

  if (try_exec_builtin(cmd) == 1)
  {

    // If it's not a built-in command, execute it as an external command.
    exec_external_cmd(cmd);
  }
}

/** Execute built-in commands
 *
 * If the command is a built-in command, execute it and return 1 if appropriate
 * If the command is not a built-in command, do nothing and return 0
 */
int try_exec_builtin(struct Command *cmd)
{

  if (strcmp(cmd->args[0], "exit") == 0)
  {
    if (cmd->args[1] != NULL)
    {
      fprintf(stderr, "An error has occurred\n");
    }
    exit(0);
  }
  else if (strcmp(cmd->args[0], "cd") == 0)
  {

    if (cmd->args[1] == NULL)
    {
      fprintf(stdout, "An error has occurred\n");
      fprintf(stderr, "An error has occurred\n");

      return 0;
    }
    else if (cmd->args[2] != NULL)
    {
      fprintf(stderr, "An error has occurred\n");
    }
    else
    {

      if (chdir(cmd->args[1]) != 0)
      {

        fprintf(stderr, "An error has occurred\n");

        return 0;
      }
    }
    return 0;
  }
  else if (strcmp(cmd->args[0], "path") == 0)
  {
    // each argument is a different path
    //  we put all of these into an array -- remove first arg
    char **newArgs = cmd->args + 1;
    set_shell_path(newArgs);
    for (int i = 0; i < MAX_ENTRIES_IN_SHELLPATH; ++i)
    {
      if (shell_paths[i][0] == '\0')
      { // If the first character is NULL, skip
        break;
      }
    }
    return 0;
  }

  return 1;
}

void exec_external_cmd(struct Command *cmd)
{

  pid_t pid;

  char *path = cmd->args[0];
  int num_bytes = strlen(path);
  if (path[num_bytes - 1] == '\n')
  {
    path[num_bytes - 1] = '\0';
  }

  if (is_absolute_path(path) == 0)
  {

    // If the path is not absolute, we need to search for it in the shell path
    for (int i = 0; i < MAX_ENTRIES_IN_SHELLPATH; ++i)
    {

      if (shell_paths[i][0] == '\0')
      {
        fprintf(stderr, "An error has occurred\n");

        return;
      }

      char *full_path = malloc(strlen(shell_paths[i]) + strlen(path) + 3);
      if (full_path == NULL)
      {
        fprintf(stderr, "An error has occurred\n");
        exit(1);
      }
      char *new_str = malloc(strlen(shell_paths[i]) + 2);
      if (new_str == NULL)
      {
        fprintf(stderr, "An error has occurred\n");
        free(full_path); // After using full_path, you can free it.
        exit(1);
      }

      int num_bytes = strlen(shell_paths[i]);
      if (shell_paths[i][num_bytes - 1] == '\n')
      {
        shell_paths[i][num_bytes - 1] = '\0';
      }

      strcpy(new_str, shell_paths[i]);
      strcat(new_str, "/");

      full_path = exe_exists_in_dir(new_str, path, 0);

      if (full_path != NULL)
      {

        path = full_path;
        // free(full_path); // After using full_path, you can free it.
        free(new_str);
        break;
      }
    }
  }

  // fprintf(stderr, ("%s Path is absolute\n", path));

  pid = fork();

  if (pid < 0)
  {
    // Fork failed
    fprintf(stderr, "An error has occurred\n");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) // This block will be executed by the child process
  {
    if (cmd->outputFile != NULL)
    {
      int fd;

      if ((fd = open(cmd->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
      {
        fprintf(stderr, "An error has occurred\n");
        exit(1);
      }
      if (dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1)
      {
        fprintf(stderr, "An error has occurred\n");
        if (close(fd) == -1)
        {
          fprintf(stderr, "An error has occurred\n");
          exit(EXIT_FAILURE);
        }
        exit(1);
      }

      if (close(fd) == -1)
      {
        fprintf(stderr, "An error has occurred\n");
        exit(EXIT_FAILURE);
      }
    }
    if (execv(path, cmd->args) == -1)
    {
      fprintf(stderr, "An error has occurred\n");

      exit(1);
    }
  }
  else // This block will be executed by the parent process
  {
    int status;
    if (waitpid(pid, &status, 0) == -1)
    {
      fprintf(stderr, "An error has occurred\n");
      exit(EXIT_FAILURE);
    }
  }
}

bool is_only_whitespace(const char *str)
{

  while (*str != '\0')
  {
    if (!isspace((unsigned char)*str))
    {
      return false;
    }
    str++;
  }
  return true;
}

void exec_file(struct Command *cmd)
{

  FILE *fp = fopen(cmd->args[0], "r");

  if (fp == NULL)
  {
    fprintf(stderr, "An error has occurred\n");
    exit(EXIT_FAILURE);
  }

  char *line = NULL;
  size_t len = 0;
  int count = 0;
  while (getline(&line, &len, fp) != -1)
  {
    char *line_copy = strdup(line);
    if (line_copy == NULL)
    {
      fprintf(stderr, "An error has occurred\n");
      exit(EXIT_FAILURE);
    }
    if (is_only_whitespace(line_copy))
    {
      continue;
    }

    count += 1;

    if (exec_line(line) == 1)
    {
      exit(0);
    }
  }
  if (count == 0)
  {
    fprintf(stderr, "An error has occurred\n");

    exit(1);
  }

  if (fclose(fp) == EOF)
  {
    fprintf(stderr, "An error has occurred\n");
    exit(EXIT_FAILURE);
  }

  // free the allocated line buffer
  if (line)
  {
    free(line);
  }

  exit(0);
}