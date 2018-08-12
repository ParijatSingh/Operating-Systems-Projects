#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

// This function parses the command taking spaces as token
void  parse(char *input, char **argv)
{
	// check until end of input
     while (*input != '\0')
	 {
		 // look for space and replace with string terminator
          while (*input == ' ')
               *input++ = '\0';
          *argv++ = input;
          while (*input != '\0' && *input != ' ')
               input++;
     }
	 /* terminate argument array */
     *argv = '\0';
}

// This is the function to parse the input. It splits the commands by '|' token
void  parsecommands(char *input, char **command, int *commandcount)
{
     while (*input != '\0')
	 {
		while (*input == '|')
		{
			// use pipe '|' as token, replace with 0
			*input++ = '\0';
		    (*commandcount)++;
		}
        *command++ = input;
        while (*input != '\0' && *input != '|') {
            input++;
		}
     }
	 /* terminate command array */
     *command = '\0';
}

/* function to trim leading and trailing whitespaces*/
void trim(char *input)
{
   char *dst = input, *src = input;
   char *end;
   /* skip leading spaces */
   while (isspace((unsigned char)*src))
   {
      ++src;
   }
   /* remove trailing spaces */
   end = src + strlen(src) - 1;
   while (end > src && isspace((unsigned char)*end))
   {
      *end-- = 0;
   }
   /* copy src to dest if the whitespaces were removed */
   if (src != dst)
   {
      while ((*dst++ = *src++));
   }
}

/* function to execute a command in pipe. fdin will be 0 for first command*/
int execute (int fdin, int fdout, char **cmd)
{
  pid_t pid;
  int status;
  int exstatus; /* execution status */
  if ((pid = fork ()) == 0)
  {
      if (fdin != 0)
      {
          dup2 (fdin, 0);
          close (fdin);
      }
      if (fdout != 1)
      {
          dup2 (fdout, 1);
          close (fdout);
	  }

      exstatus = execvp (*cmd,cmd);
	  // print error when command failed to execute
	  if(exstatus == -1)
	  {
		  fprintf(stderr, "Error executing. Please check if command '%s' is valid.\n", *cmd);
		  exit(EXIT_FAILURE);
	  }
    }else
	{
		/* parent wait for completion  */
        while (wait(&status) != pid);
		/* child is done now - close the read end fd */
		close(fdin);
  }
  return exstatus;
}

/*  This function loops through all the commands in pipe and executes them.*/
int executepipe (char** commands, int n)
{
  int i;
  int fdin, fd [2];
  // assuming that no. of arguments per command will not exceed 5
  char *argv[5];

  /* The first process should get its input from the original file descriptor 0.  */
  fdin = 0;

  /* Pipe and process all commands but the last stage of the pipeline.  */
  for (i = 0; i < n - 1; ++i)
    {
      pipe (fd);

      /* fd [1] is the write end of the pipe, fdin is carried from the last loop. */
	  parse(commands[i], argv);
      execute (fdin, fd [1], argv);

      /*The child will write here.  */
      close (fd [1]);

      /* The next child will read from there.  */
      fdin = fd [0];
    }

  /* Last stage of the pipeline - set stdin be the read end of the previous pipe
     and output to the original file descriptor 1. */
  if (fdin != 0)
    dup2 (fdin, 0);

  /* Execute the last stage with the current process. */
  parse(commands[n-1], argv);
  execute(fdin, 1, argv);
  return 0;
}

/* Function to add an entry in the history list.
Moves all the entries by 1 position if it exceeds 100 thus removing the earliest entry */
void addhistory(char **history, char *command, int histsize)
{
	//create a copy of command for storing in the list
	char* cmd = (char*) malloc(strlen(command) + 1);
	strcpy (cmd, command);

	//printf("adding to history\n");
	if(histsize == 100)
	{
		for (int i = 0; i<100; i++)
		{
			history[i]=history[i+1];
		}
		history[99] = cmd;
	}else
	{
		history[histsize]=cmd;
	}
}

/* Function to print the history. If offset is 0 then all the history will be printed */
void printhistory(char **history, int offset, int histsize)
{
	if(offset == 0)
	{
		for (int i = 0; i<histsize; i++)
		{
			printf("%d %s\n", i+1 , history[i]);
		}
	}else
	{
		printf("%d %s\n", offset , history[(offset-1)] );
	}
}

int main()
{
  char input[512];
  // set max number of commands in pipe to 32
  char *commands[32];
  char  *argv[32];
  // history of commands is limited to last 100 commands
  char  *hist[100];
  // histentries - number of entries in the history: if it reaches 100 the increment will stop
  int histentries=0;
  //printf("PID = %d\n", getpid());
  // store the stdin  to restoration later after the command execution
  int orig_rd_fd = dup(0);
  while (1)
    {

	  //memset (input, 0, 512 );
	  int commandcount = 1;
      // Show prompt.
      printf("$ ");
	  // read the stdio into input buffer
	  gets(input);
	  // save the input, even if it is invalid
	  addhistory(hist, input, histentries);
	  if(histentries < 100) histentries++;
	  // now parse the input and convert to argument array
	  parsecommands(input, commands, &commandcount);
	  for(int i=0; i<commandcount; i++)
	  {
		trim(commands[i]);
		//printf("Command %d=%s\n", i, commands[i]);
	  }
	  if(commandcount == 1){
		  parse(commands[0], argv);
		  // check if the command is exit
		  if(strcmp(argv[0], "exit") == 0)
		  {
				  return 0;
		  // check for change directory command and call chdir command
		  }else if(strcmp(argv[0], "cd") == 0)
		  {
			  chdir(argv[1]);
		  // check for history command
		  }else if(strcmp(argv[0], "history") == 0)
		  {
			if(argv[1]!='\0')
			{
				// clear history list if -c option is encountered
				if(strcmp(argv[1], "-c") == 0){
					memset(&hist[0], 0, sizeof(hist));
					histentries = 0;
				// otherwise print history
				}else
				{
					printhistory(hist, atoi(argv[1]), histentries);
				}
			}else
			{
				printhistory(hist, 0, histentries);
			}
		  }else
		  {
			// if single command then execute it with stdin and stdout file descriptors
			execute(0, 1, argv);
		  }
	  }else
	  {
		// Multiple commands are to be executed sequentially in child processes
		executepipe(commands, commandcount);
	  }
	  // finally restore the read fd to original stdin
	  dup2(orig_rd_fd, 0);
	}
   return 0;
}
