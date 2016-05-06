#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

/* BUFFERS */
#define BUFFER_STREAM	1
#define BUFFER_SIZE		4096

/* SIZES */
#define FILE_NAME_SIZE 256
#define THREAD_SIZE 256 * 256

 /*
 *	Function prototypes
 */
void openingStyle2(void);

void fileCheck(char *, char *);

void *searchInFile(void *);

void writePipe(const int, const int);

void writeLogFile(const char *);


struct _searchParameters
{
	char *filePath;
	char *fileName;
	char *searchingWord;

};


int main(int argc, char *argv[])
{
	int countChild;

	/* Usage */
	if (argc != 3)
	{
		printf("Using: ./grD [directory] text\n");
		return 1;
	}


	fileCheck(argv[1], argv[2]);

	while(0 < wait(NULL))
	{
		countChild++;
		exit(EXIT_SUCCESS);
	}

	printf("%d\n", countChild);

	return 0;
} /* end main function */

/***************************************************

	FUNCTION:	fileCheck

	SUPPOSE:

	COMMENT:

		Olcak olacak olacak o kadar..

****************************************************/
void fileCheck(char *currentPath, char *searchText)
{
	/* Buffer */
	char buffer[BUFFER_SIZE];

	/* Create thread and struct initialize*/
	struct _searchParameters parametersSearchInfile;
	pthread_t thread[THREAD_SIZE];
	int currentThread = 0;

	/* Folder variables */
	DIR *dir;
	struct dirent *ent;

	/* Fork variable */
	pid_t childPid;

	/* Try to open folder */
	if ((dir = opendir(currentPath)) == NULL)
	{
		perror("Error 122");
		exit(EXIT_FAILURE);
	}
	else /* Successfull open dir */
	{
		/* Reading directioary */
		while ((ent = readdir(dir)) != NULL)
		{

			/* NO CWD and upper */
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
				continue;

			if (ent->d_type == DT_DIR)
			{
				if ((childPid = fork()) < 0)
				{
					perror("Error 378");
					exit(EXIT_FAILURE);
				}

				if (childPid == 0)
				{
					/* Create new path */
					char newPath[FILE_NAME_SIZE];
					strcpy(newPath, currentPath);
					strcat(newPath, "/");
					strcat(newPath, ent->d_name);

					/* Filecheck new path */
					fileCheck(newPath, searchText);
				}

			} /* end if DT_DIR */

			if (ent->d_type == DT_REG) /* Is this file */
			{

				parametersSearchInfile.filePath = currentPath;
				parametersSearchInfile.fileName = ent->d_name;
				parametersSearchInfile.searchingWord = searchText;

				currentThread++;
				pthread_create(&thread[currentThread], NULL, searchInFile, &parametersSearchInfile);
				pthread_join(thread[currentThread], NULL);

			} /* end if else (dirent type) */

		} /* end while */

		closedir(dir);

	} /* end if else (opendir) */

} /* enf function (chechFile) */

/***************************************************

	FUNCTION:	searchInFile

	SUPPOSE:	Searching text in file

	COMMENT:

		Olcak olacak olacak o kadar..

****************************************************/
void *searchInFile(void *parametersSearchInfile)
{
	/* Parameters */
	char filePath[FILE_NAME_SIZE];
	char fileName[FILE_NAME_SIZE];
	char searchingWord[FILE_NAME_SIZE];

	/* Thread import parameter data */
	struct _searchParameters * parameters = (struct _searchParameters *)parametersSearchInfile;

	strcpy(filePath, parameters->filePath);
	strcpy(fileName, parameters->fileName);
	strcpy(searchingWord, parameters->searchingWord);

	/* Variables */
	int currentLineNumber = 1,		/* currentLineNumber is line counter */
		curentColumnNumber = 1,		/* curentColumnNumber is column counter */
		countLetter = 0,			/* countLetter need to while loop */
		totalWord = 0;				/* totalWord find searching text in the file */

	char currentChar;

	/* Temp file variables */
	char tempFileName[FILE_NAME_SIZE];

	/* Create file path */
	char newPath[FILE_NAME_SIZE];
	strcpy(newPath, filePath);
	strcat(newPath, "/");
	strcat(newPath, fileName);


		/*
		*	Openin temp file for result
		*/
		snprintf(tempFileName, sizeof(tempFileName), "%ld.txt", (long)getpid());
		FILE *tempFileDescriptor = fopen(tempFileName, "a+");

		/* Write header for result temp file */
		fprintf(tempFileDescriptor, "Path: %s\nFile: %s -> %s\n", filePath, fileName, searchingWord);

	/* Opening file for searching (READ ONLY MODE) */
	int openFileForReadingHandle = open(newPath, O_RDONLY);

	if (openFileForReadingHandle == -1)
	{
		printf("Error for opening file.\nExiting..\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		int countArgv = 0;

		while (read(openFileForReadingHandle, &currentChar, BUFFER_STREAM) > 0)
		{
			if (currentChar != '\0')
				++curentColumnNumber;

			if (currentChar == '\n')
			{
				++currentLineNumber;
				curentColumnNumber = 1;
			}

			/* Control word */
			if (currentChar == searchingWord[countArgv])
			{
				++countLetter;
				++countArgv;

				if (countLetter == strlen(searchingWord))
				{
					++totalWord;

					/* Results are writing to temp file. */
					fprintf(tempFileDescriptor,
							"\t%d line, %d column found.\n",
							currentLineNumber,
							curentColumnNumber - (int)strlen(searchingWord)
					);

					/* Must be zero */
					countLetter = 0;
				}
			}
			else
			{
				countLetter = 0;
				countArgv = 0;
			}

		} /* end while */

		/* Result controls for not results */
		if (totalWord == 0)
		{
			fprintf(tempFileDescriptor, "\nNot found in file\n\n");
		}

		/* End of file */
		fprintf(tempFileDescriptor, "-------------------------------------------------------------\n");

		/* Print Screen */
		printf("%s \t %d times found.\n", fileName, totalWord);

		/* Close tempfile */
		fclose(tempFileDescriptor);

		/* Close reading file */
		close(openFileForReadingHandle);

	} /* end if else */

	/* Delete temp file */
	/*
	unlink(tempFileDescriptor);
	*/

	return 0;

} /* end function searchInFile */

/***************************************************

	FUNCTION:	writeLogFile

	SUPPOSE:	Searching text in file

	COMMENT:

		Olcak olacak olacak o kadar..

****************************************************/
void writeLogFile(const char *results)
{
	int logFileHandle = open("gfD.log", O_CREAT | O_WRONLY | O_APPEND);
	write(logFileHandle, results, strlen(results));
	close(logFileHandle);

	return;
}

/***************************************************

	FUNCTION:	writePipe

	SUPPOSE:	Copy from temp file to pipe

	COMMENT:

		Olcak olacak olacak o kadar..

****************************************************/
void writePipe(const int tempFileDescription, const int pipeFileHandle)
{
	/* Variable */
	char tempText[BUFFER_SIZE];

	read(tempFileDescription, tempText, BUFFER_SIZE);
	write(pipeFileHandle, tempText, BUFFER_SIZE);

	return;
}

/***************************************************

	FUNCTION:	Opening

	SUPPOSE:	Program opening style

	COMMENT:

		

****************************************************/
void openingStyle2(void)
{                                                                                
printf("   ggggggggg   gggggrrrrr   rrrrrrrrr       eeeeeeeeeeee    ppppp   ppppppppp  \n");
usleep(200000);
printf("  g:::::::::ggg::::gr::::rrr:::::::::r    ee::::::::::::ee  p::::ppp:::::::::p  \n");
usleep(200000);
printf(" g:::::::::::::::::gr:::::::::::::::::r  e::::::eeeee:::::eep:::::::::::::::::p \n");
usleep(200000);
printf("g::::::ggggg::::::ggrr::::::rrrrr::::::re::::::e     e:::::epp::::::ppppp::::::p\n");
usleep(200000);
printf("g:::::g     g:::::g  r:::::r     r:::::re:::::::eeeee::::::e p:::::p     p:::::p\n");
usleep(200000);
printf("g:::::g     g:::::g  r:::::r     rrrrrrre:::::::::::::::::e  p:::::p     p:::::p\n");
usleep(100000);
printf("g:::::g     g:::::g  r:::::r            e::::::eeeeeeeeeee   p:::::p     p:::::p\n");
usleep(100000);
printf("g::::::g    g:::::g  r:::::r            e:::::::e            p:::::p    p::::::p\n");
usleep(100000);
printf("g:::::::ggggg:::::g  r:::::r            e::::::::e           p:::::ppppp:::::::p\n");
usleep(100000);
printf(" g::::::::::::::::g  r:::::r             e::::::::eeeeeeee   p::::::::::::::::p \n");
usleep(200000);
printf("  gg::::::::::::::g  r:::::r              ee:::::::::::::e   p::::::::::::::pp  \n");
usleep(200000);
printf("    gggggggg::::::g  rrrrrrr                eeeeeeeeeeeeee   p::::::pppppppp    \n");
usleep(300000);
printf("            g:::::g                                          p:::::p            \n");
usleep(300000);
printf("gggggg      g:::::g                                          p:::::p            \n");
usleep(300000);
printf("g:::::gg   gg:::::g                                         p:::::::p           \n");
usleep(300000);
printf(" g::::::ggg:::::::g                                         p:::::::p           \n");
usleep(300000);
printf("  gg:::::::::::::g                                          p:::::::p           \n");
usleep(400000);
printf("    ggg::::::ggg                                            ppppppppp           \n");
usleep(400000);
printf("       gggggg                                                                   \n");
printf("\n");
printf(" #HW04                                                   Created by Ozan YILDIZ \n");
usleep(500000);
printf("\n");
}
