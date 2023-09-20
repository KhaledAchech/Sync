#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RL_BUFSIZE 1024
#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"
//-------------------------------------------------

/*
  Function Declarations for builtin shell commands:
*/
int sync_cd(char **args);
int sync_help(char **args);
int sync_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
*/
char *builtin_str[] = {
    "cd",
    "help",
    "exit"};

int (*builtin_func[])(char **) = {
    &sync_cd,
    &sync_help,
    &sync_exit};

int sync_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}

int sync_cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "sync: expected argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("sync");
    }
  }
  return 1;
}

int sync_help(char **args)
{
  int i;
  printf("SYNC Help section:\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < sync_num_builtins(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int sync_exit(char **args)
{
  return 0;
}

int sync_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process
    if (execvp(args[0], args) == -1)
    {
      perror("sync");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("sync");
  }
  else
  {
    // Parent process
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int sync_execute(char **args)
{
  int i;

  if (args[0] == NULL)
  {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < sync_num_builtins(); i++)
  {
    if (strcmp(args[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(args);
    }
  }

  return sync_launch(args);
}

char *sync_read_line()
{
  int bufsize = RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer)
  {
    fprintf(stderr, "sync: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // Read a character
    c = getchar();

    if (c == EOF)
    {
      exit(EXIT_SUCCESS);
    }
    else if (c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize)
    {
      bufsize += RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        fprintf(stderr, "sync: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}
char **sync_split_line(char *line)
{
  int bufsize = TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token, **tokens_backup;

  if (!tokens)
  {
    fprintf(stderr, "sync: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOK_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;

    if (position >= bufsize)
    {
      bufsize += TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens)
      {
        free(tokens_backup);
        fprintf(stderr, "sync: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
}

void loop()
{
  char *line;
  char **args;
  int status;

  do
  {
    printf("(sync) ");
    line = sync_read_line();
    args = sync_split_line();
    status = sync_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main()
{
  loop();
  return EXIT_SUCCESS;
}
