/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: hnoyt
 *
 * Created on April 5, 2016, 11:03 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Buffers
#define BUFFER_MINI 1
#define BUFFER_STANDART 2048

#define MAX_PATH 256

void openDirectory(const char *, const char *);
int searchInFile(const char *, const char*, const char *, int);

void writingLog(const char*);

// Pipe functions
void pipeWriting(const int, const char *);
void pipeReading(const int);

int main(int argc, char const *argv[])
{
	/*
	if (argc != 3)
	{
	printf("Usage: ./grepFromDir [directory] [searchInFile text}\n");
	exit(EXIT_FAILURE);
	}
	*/

	openDirectory(argv[1], argv[2]);

	return 0;
}

//
//   FUNCTION:	openDirectory
//
//   PURPOSE:	Checks the files.
//
//   COMMENTS(TR):
//
//		Amaç dosyları kontrol etmek ve fork yapmaktır.
//		Her fork için pipe yapılır.
//		Önce fork işlemi yapılır daha sonra ise child process
//		dosya mı klasör mü olduğuna karar verir. Eğer dosya ise arama yapar ancak
//		folder ise bu sefer recursive yaparak fonksiyonu tekrar çağırır kendi alt processlerine
//		yeni path gönderir. İşlem aynı şekilde tekrarlanır.
//
void openDirectory(const char *path, const char *text)
{
	// Folder variables
	DIR *directory;
	struct dirent *entry;

	// Fork variable
	pid_t pid;

	// Pipe variable
	int fileDescription[2];

	if ((directory = opendir(path)) == NULL)
	{
		perror("opendir");
		exit(EXIT_FAILURE);
	}
	else
	{
		while ((entry = readdir(directory)) != NULL)
		{

			// First pipe arter fork
			if ((pipe(fileDescription) < 0) || ((pid = fork()) < 0))
			{
				perror("pipe");
				perror("fork");
				exit(EXIT_FAILURE);
			}
			else if (pid) // Parent process
			{
				// Waiting child process
				wait(NULL);
				char readText[BUFFER_STANDART];

				close(fileDescription[1]);
				read(fileDescription[0], &readText, sizeof(readText));

				close(fileDescription[0]);
				writingLog(readText);
			}
			else // Child process
			{

				// NO CWD and upper
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					exit(EXIT_FAILURE);

				// Control: Is this folder?
				if (entry->d_type == DT_DIR)
				{
					// Create new folder path
					char newFolderPath[MAX_PATH];
					strcpy(newFolderPath, path);
					strcat(newFolderPath, "/");
					strcat(newFolderPath, entry->d_name);

					// openDirectory new path
					openDirectory(newFolderPath, text);
					exit(EXIT_SUCCESS);
				}


				// Control: Is this file?
				else if (entry->d_type == DT_REG)
				{
					// Pipe and fork variable
					close(fileDescription[0]);

					// We search text in file
					searchInFile(path, entry->d_name, text, fileDescription[1]);

					close(fileDescription[1]);
					exit(EXIT_SUCCESS);
				}
			}

		} // end while

	} // end if else

} // end function

//
//   FUNCTION:	searchInFile
//
//   PURPOSE:	Search file and send with pipe
//
//   COMMENTS(TR):
//
//		Aldığı filePath ve fileName 
//		Aldığı filePath argümanı ile ilgili dosyayı READONLY modunda açar.
//		Daha sonra açmış olduğu bu dosyadan BUFFER_MINI miktarı kadar okur.
//		BUFFER_MINI miktarı default olarak 1 olarak ayarladım.
//		Her bir byte pipe dosyasına yazılır ve dosya kapatılır.
//
int searchInFile(
	const char* filePath, 
	const char* fileName, 
	const char *searchInFileText, 
	int fileDescription)
{
	// Create new folder path
	char newPath[MAX_PATH];
	strcpy(newPath, filePath);
	strcat(newPath, "/");
	strcat(newPath, fileName);

	// Control
	printf("%s/%s - %d - Parent: %d\n", filePath, fileName, getpid(), getppid());

	// Variables
	int currentLineNumber = 1,		// currentLineNumber is line counter
		curentColumnNumber = 1,		// curentColumnNumber is column counter
		countLetter = 0,			// countLetter need to while loop
		totalWord = 0;				// totalWord find searchInFile text in the file

	char currentChar;

	// Opening file (READ ONLY MODE)
	int openFileForReading = open(newPath, O_RDONLY);

	// Opening temp file for result
	char tempfileName[BUFFER_STANDART];
	snprintf(tempfileName, sizeof(tempfileName), "%d.txt", getpid());
	int tempFileForWriting = open(tempfileName, O_CREAT | O_WRONLY | O_APPEND);

		// Create result header text for temp file
		char tempResultText[BUFFER_STANDART];
		snprintf(
		tempResultText, 
		sizeof(tempResultText),
		"Path: %s\n%s -> %s file\n",
			filePath, 
			searchInFileText, 
			fileName
		);
		write(tempFileForWriting, tempResultText, strlen(tempResultText));


	// We're starting to read file
	if (openFileForReading == -1)
	{
		printf("Error for opening file.\nExiting..\n");
		return 1;
	}
	else
	{
		int countArgv = 0;

		while (read(openFileForReading, &currentChar, BUFFER_MINI) > 0)
		{
			if (currentChar != '\0') ++curentColumnNumber;
			if (currentChar == '\n')
			{
				++currentLineNumber;
				curentColumnNumber = 1;
			}

			// Control word
			if (currentChar == searchInFileText[countArgv])
			{
				++countLetter;
				++countArgv;

				if (countLetter == strlen(searchInFileText))
				{
					++totalWord;

					// Create result string for write temp file
					snprintf(
						tempResultText, 
						sizeof(tempResultText),
						"\t%d line, %zu column found.\n",
							currentLineNumber, 
							curentColumnNumber - strlen(searchInFileText)
						);

					// We're writing result string in temp file
					write(tempFileForWriting, tempResultText, strlen(tempResultText));

					// Must be zero
					countLetter = 0;
				}
			}
			else
			{
				countLetter = 0;
				countArgv = 0;
			}
		} // end while

		// Send pipe all results
		char reagent[] = "----------\n";
		write(tempFileForWriting, reagent, strlen(reagent));
		close(tempFileForWriting);

		// Send pipe all result
		char asd[BUFFER_STANDART];
		int fd = open(tempfileName, O_RDONLY);
		if (read(tempFileForWriting, asd, BUFFER_STANDART) < 0)
			perror("error");
		
		// Control
		//printf("%s\n", asd);

		write(fileDescription, asd, strlen(asd) + 1);
		close(fd);

 		// If no results, delete temp file
		if (totalWord == 0)
			unlink(tempfileName);

		// Close reading file
		close(openFileForReading);

	}

	return 0;
}

//
//   FUNCTION:	writingLog
//
//   PURPOSE:	Parent write all result 
//
//   COMMENTS(TR):
//
//		main() fonksiyondaki parent pipe ve fifolardan gelen herşeyi log file yazar
//
void writingLog(const char* text)
{
	int logFileForWriting;
	
	if ((logFileForWriting = open("gfD.log", O_CREAT | O_WRONLY | O_APPEND)) < 0)
		perror("open");

	write(logFileForWriting, text, strlen(text));

	close(logFileForWriting);
}

//
//   FUNCTION:	pipeWriting
//
//   PURPOSE:	Writing with pipe
//
//   COMMENTS(TR):
//
//		Aldığı path argümanı ile ilgili dosyayı READONLY modunda açar.
//		Daha sonra açmış olduğu bu dosyadan BUFFER_MINI miktarı kadar okur.
//		BUFFER_MINI miktarı default olarak 1 olarak ayarladım.
//		Her bir byte pipe dosyasına yazılır ve dosya kapatılır.
//
void pipeWriting(const int pipeFile, const char *text)
{
	write(pipeFile, text, strlen(text)); // Pipe dosyasına yazacak

	return;
}

//
//   FUNCTION:	pipeReading
//
//   PURPOSE:	Reading with pipe
//
//   COMMENTS(TR):
//
//		Dosyadan okumayı sağlar pipe ile 
//
void pipeReading(const int pipeFile)
{
	//read(pipeFile);

	return;
}