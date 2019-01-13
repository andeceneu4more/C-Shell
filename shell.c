#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include "searchHistory.h"

int exitSh (char** args);
int cdSh (char** args);
int helpSh (char** args);

pid_t *suspendedChildren = NULL;
int suspendedChildrenSize = 0;
pid_t currenthild = 0;

void sigHandler (int signo) 
{
	switch (signo) 
	{
  		case SIGINT:  /* Incoming SIGINT */
			if (currentChild) 
			{
				printf ("\n");
				kill(currentChild, SIGINT);
				currentChild = 0;
			}   			
    		break;

  		case SIGQUIT:  /* Incoming SIGQUIT */
			if (currentChild) 
			{
				suspendedChildrenSize++;
				suspendedChildren = (pid_t*)realloc (suspendedChildren, suspendedChildrenSize * sizeof (pid_t));
				suspendedChildren[suspendedChildrenSize - 1] = currentChild;

				printf ( "\n[%d]+ Stopped\t\t%d\n", suspendedChildrenSize, currentChild); 
				kill(currentChild, SIGSTOP);
				currentChild = 0;
			} 
    		break;
  		default: break;
  	}
	return;
}

void resumeProcess ()
{
	//inca nu functioneaza	
	if (!suspendedChildrenSize)
	{
		fprintf (stderr, "no such job\n");	
	}
	else
	{
		printf ("%d\n", suspendedChildren[suspendedChildrenSize - 1]);
		kill (suspendedChildren[suspendedChildrenSize - 1], SIGCONT);
	}
	exit (EXIT_SUCCESS);
}

char* builtinStruct[] =
{
	"exit",
	"cd",
	"help"
};

int (*builtinFunction[]) (char**) =
{
	&exitSh,
	&cdSh,
 	&helpSh
};

int builtinNr()
{
	return sizeof(builtinStruct) / sizeof(char*);
}

int exitSh(char** args)
{
	exit (EXIT_SUCCESS);
}

int cdSh (char** args)
{
	if (args[1] == NULL)
	{
		fprintf (stderr, "expected argument to \"cd\"\n");
	}
	else
	{
		if (chdir (args[1]))
		{
			fprintf (stderr, "chdir error\n");
		}
	}

	return 1;
}

int helpSh (char** args)
{
	int i;
	printf ("Proiect Sisteme de Operare\n");
	printf ("Autori: Manea Andrei & Gidea Andrei\n");
	printf ("The following functions are builted in:\n");
	for (i = 0; i < builtinNr(); i++)
		printf (" %s\n", builtinStruct[i]);
	return 1;
}


char* readLine()
{
	char *line = NULL;
	char *buffer = malloc(sizeof(char) * MAX_BUFFER_SIZE);
	fgets(buffer, MAX_BUFFER_SIZE, stdin);
	int bufSize = strlen(buffer);
	line = (char *) malloc(sizeof(char) * bufSize);
	if (line == NULL)
	{
		fprintf(stderr, "alocation error Buffer Size %d\n", bufSize);
		exit(EXIT_FAILURE);
	}
	strcpy(line, buffer);
	free(buffer);
	return line;
}

char** parse(char* line)
{
	int bufSize = ARGS_BUFFER_SIZE, poz = 0;
	char** args = malloc(sizeof(char*) * bufSize);
	char* arg;
	if (!args)
	{
		fprintf(stderr, "allocation error Parse\n");
		exit(EXIT_FAILURE);
	}
	arg = strtok(line, ARGS_DELIM);
	while (arg != NULL)
	{
		args[poz] = arg;
		poz++;

		if(poz >= bufSize)
		{
			bufSize += ARGS_BUFFER_SIZE;
			args = realloc(args, sizeof(char*) * bufSize);
			if(!args)
			{
				fprintf(stderr, "allocation error Parse\n");
				exit(EXIT_FAILURE);
			}
		}

		arg = strtok(NULL, ARGS_DELIM);
	}
	args[poz] = NULL;
	return args;
}

int launchProcess(char** args)
{
	pid_t pid, wpid;
	int status;
	pid = fork();
	currentChild = pid;
	if (pid == 0)
	{
		//child process
		if (args[0][0] == '!')
		{
			// history command
			execHistory(args);
		}
		else if (strcmp(args[0], "fg") == 0)
			resumeProcess ();
		else
		{
			if(execvp (args[0], args) == -1)
			{
				fprintf (stderr, "launchProcess execvp\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	else if (pid < 0)
	{
		//error forking
		fprintf (stderr, "launchProcess forking\n");
	}
	else
	{
		//parent process
		do
		{
			wpid = waitpid (pid, &status, WUNTRACED);
		}
		while (!WIFEXITED (status) && !WIFSIGNALED (status));
	}

	return 1;
}

int executeLine(char** args)
{
	int i;
	if(args[0] == NULL)
	{
		//empty command
		return 1;
	}

	for(i = 0; i < builtinNr(); i++)
	{
		if(strcmp(args[0], builtinStruct[i]) == 0)
		{
			return (*builtinFunction[i])(args);
		}
	}

	return launchProcess(args);
}

void loop()
{
	char *line;
	char **args;
	int state;
	char *currDir = (char*) malloc (sizeof (char) * PATH_MAX);
	if (!getcwd (currDir, sizeof (char) * PATH_MAX))
	{
		fprintf(stderr, "getcwd() error\n");
		exit(EXIT_FAILURE);
	}
	hisPath = (char*) malloc (sizeof (char) * PATH_MAX);
	sprintf(hisPath, "%s/history.cshrc", currDir);
	do
	{

		// print the path
		printf ("~%s$ ", currDir);

		line = readLine();
		// A Manea
		nrCommands = addLine(line);
		// tratarea de erori
		// A Manea
		args = parse(line);
		// A Gidea
		state = executeLine(args);

		if (!getcwd (currDir, sizeof (char) * PATH_MAX))
		{
			fprintf(stderr, "getcwd() error\n");
			exit(EXIT_FAILURE);
		}
		// A Gidea
	}
	while (state);

	free(currDir);
	free(hisPath);
}

int main()
{
	if (signal (SIGINT, sigHandler) == SIG_ERR)
    		printf("Parent: Unable to create handler for SIGINT\n");

  	if (signal (SIGQUIT, sigHandler) == SIG_ERR)
    		printf("Parent: Unable to create handler for SIGQUIT\n");
	loop();
	return EXIT_SUCCES;
}
