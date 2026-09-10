#include "shell.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

bool handleRedirection(char* args[], int& argc)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Invalid redirection\n");
                return false;
            }

            int fd = open(args[i + 1], O_RDONLY);

            if (fd < 0)
            {
                perror("open");
                return false;
            }

            dup2(fd, STDIN_FILENO);
            close(fd);

            for (int j = i; j + 2 < argc; j++)
                args[j] = args[j + 2];

            argc -= 2;
            i--;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Invalid redirection\n");
                return false;
            }

            int fd = open(args[i + 1],
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);

            if (fd < 0)
            {
                perror("open");
                return false;
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);

            for (int j = i; j + 2 < argc; j++)
                args[j] = args[j + 2];

            argc -= 2;
            i--;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Invalid redirection\n");
                return false;
            }

            int fd = open(args[i + 1],
                          O_WRONLY | O_CREAT | O_APPEND,
                          0644);

            if (fd < 0)
            {
                perror("open");
                return false;
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);

            for (int j = i; j + 2 < argc; j++)
                args[j] = args[j + 2];

            argc -= 2;
            i--;
        }
    }

    args[argc] = nullptr;
    return true;
}

void saveStandardDescriptors()
{
    savedStdin = dup(STDIN_FILENO);
    savedStdout = dup(STDOUT_FILENO);
}

void restoreStandardDescriptors()
{
    if (savedStdin != -1)
    {
        dup2(savedStdin, STDIN_FILENO);
        close(savedStdin);
        savedStdin = -1;
    }

    if (savedStdout != -1)
    {
        dup2(savedStdout, STDOUT_FILENO);
        close(savedStdout);
        savedStdout = -1;
    }
}