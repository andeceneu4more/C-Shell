#ifndef SEARCHHISTORY_H_INCLUDED
#define SEARCHHISTORY_H_INCLUDED

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define EXIT_SUCCES 0
#define EXIT_FAILURE 1
#define MAX_BUFFER_SIZE 4096
#define ARGS_BUFFER_SIZE 128
#define ARGS_DELIM " \t\n"
char *hisPath;
int nrCommands;

int addLine(char *line)
{
	ssize_t history = open(hisPath, O_RDWR);
	if (history < 0)
	{
		fprintf(stderr, "opening history error\n");
		exit(EXIT_FAILURE);
	}
	struct stat hStat;
	if (stat(hisPath, &hStat))
	{
		fprintf(stderr, "stat history issue\n");
		exit(EXIT_FAILURE);
	}
	int historySize = 0, bufSize, i, adding = 1, ret;
	char *buffer, *p;
	line[strlen(line) - 1] = 0;
	// ignoring the '\n'
	for (i = 0; i < hStat.st_size / MAX_BUFFER_SIZE; i++)
	{
		bufSize = MAX_BUFFER_SIZE;
		// tokens with MAX_BUFFER_SIZE of history
		buffer = (char *)malloc(sizeof(char) * bufSize);
		ret = read(history, buffer, bufSize);
		if (ret < 0)
		{
			fprintf(stderr, "reading history issue\n");
			exit(EXIT_FAILURE);
		}
		p = strtok(buffer, "\n");
		while (p != NULL)
		{
			historySize++;
			while (('0' <= *p && *p <= '9'  || *p == ' ') && p != NULL)
				p++;
			//ignoring the number of command
			if (strcmp(p, line) == 0)
				adding = 0;
			// printf("%s versus %s adding %d historySize %d\n", p, line, adding, historySize);
			p = strtok(NULL, "\n");
		}
		free(buffer);
	}
	bufSize = hStat.st_size % MAX_BUFFER_SIZE;
	// final token with the remainder of history
	buffer = (char *)malloc(sizeof(char) * bufSize);
	ret = read(history, buffer, bufSize);
	if (ret < 0)
	{
		fprintf(stderr, "reading history issue\n");
		exit(EXIT_FAILURE);
	}
	p = strtok(buffer, "\n");
	while (p != NULL)
	{
		historySize++;
		while (('0' <= *p && *p <= '9'  || *p == ' ') && p != NULL)
			p++;
		//ignoring the number of command
		if (strcmp(p, line) == 0)
			adding = 0;
		// printf("%s versus %s adding %d historySize %d\n", p, line, adding, historySize);
		p = strtok(NULL, "\n");
	}
	free(buffer);
	if (adding)
	{
		historySize++;
		buffer = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
		sprintf(buffer, "%d %s\n", historySize, line);
		// print the number of the command
		ret = write(history, buffer, sizeof(char) * strlen(buffer));
		if (ret < 0)
		{
			fprintf(stderr, "printing in history issue\n");
			exit(EXIT_FAILURE);
		}
		free(buffer);
	}
	return historySize;
}

void execHistory(char **args)
{
	int number;
	ssize_t history = open(hisPath, O_RDWR);
	if (history < 0)
	{
		fprintf(stderr, "opening history error\n");
		exit(EXIT_FAILURE);
	}
	struct stat hStat;
	if (stat(hisPath, &hStat))
	{
		fprintf(stderr, "stat history issue\n");
		exit(EXIT_FAILURE);
	}
	int historySize = 1, bufSize, i, adding = 1, ret;
	char *buffer, *p;
	if (args[0][1] == '*')
	{
		for (i = 0; i < hStat.st_size / MAX_BUFFER_SIZE; i++)
		{
			bufSize = MAX_BUFFER_SIZE;
			// tokens with MAX_BUFFER_SIZE of history
			buffer = (char *)malloc(sizeof(char) * bufSize);
			ret = read(history, buffer, bufSize);
			if (ret < 0)
			{
				fprintf(stderr, "reading history issue\n");
				exit(EXIT_FAILURE);
			}
			printf("%s", buffer);
			free(buffer);
		}
		bufSize = hStat.st_size % MAX_BUFFER_SIZE;
		// final token with the remainder of history
		buffer = (char *)malloc(sizeof(char) * bufSize);
		ret = read(history, buffer, bufSize);
		if (ret < 0)
		{
			fprintf(stderr, "reading history issue\n");
			exit(EXIT_FAILURE);
		}
		printf("%s", buffer);
	}
	else
	{
		number = atoi(args[0] + 1);
		if (number < 0)
		{
			number += nrCommands;
		}
		if (number <= 0 || number > nrCommands)
		{
			fprintf(stderr, "number out command of bounds\n");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < hStat.st_size / MAX_BUFFER_SIZE; i++)
		{
			bufSize = MAX_BUFFER_SIZE;
			// tokens with MAX_BUFFER_SIZE of history
			buffer = (char *)malloc(sizeof(char) * bufSize);
			ret = read(history, buffer, bufSize);
			if (ret < 0)
			{
				fprintf(stderr, "reading history issue\n");
				exit(EXIT_FAILURE);
			}
			p = strtok(buffer, "\n");
			while (p != NULL && historySize < number)
			{
				historySize++;
				p = strtok(NULL, "\n");
			}
			if (historySize == number)
			{
				printf("%s\n", p);
				free(buffer);
				exit(EXIT_SUCCES);
			}
			free(buffer);
		}
		bufSize = hStat.st_size % MAX_BUFFER_SIZE;
		// final token with the remainder of history
		buffer = (char *)malloc(sizeof(char) * bufSize);
		ret = read(history, buffer, bufSize);
		if (ret < 0)
		{
			fprintf(stderr, "reading history issue\n");
			exit(EXIT_FAILURE);
		}
		p = strtok(buffer, "\n");
		while (p != NULL && historySize < number)
		{
			historySize++;
			p = strtok(NULL, "\n");
		}
		if (historySize == number)
		{
			printf("%s\n", p);
		}
	}
	free(buffer);
	exit(EXIT_SUCCES); //aici am facut o mica modificare. Inainte, noi ii dadeam exit
	//doar pe ramura de else. si mi se parea cod in plus sa ii dam free(buffer) si pe if
	//si pe else, asa ca i-am dat free si exit la sfarsitul functiei.
	//acum exit-ul merge cum trebuie
}
#endif
