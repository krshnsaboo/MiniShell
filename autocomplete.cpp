#include "shell.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>

static void findCommonPrefix(char matches[][PATH_MAX],
                             int matchCount,
                             char* commonPrefix)
{
    if (matchCount == 0)
    {
        commonPrefix[0] = '\0';
        return;
    }

    strcpy(commonPrefix, matches[0]);

    for (int i = 1; i < matchCount; i++)
    {
        int j = 0;

        while (commonPrefix[j] != '\0' &&
               matches[i][j] != '\0' &&
               commonPrefix[j] == matches[i][j])
        {
            j++;
        }

        commonPrefix[j] = '\0';
    }
}

static void redrawInput(const char* buffer,
                        const char* homeDirectory)
{
    printf("\r\033[K");
    displayPrompt(homeDirectory);
    printf("%s", buffer);
    fflush(stdout);
}

static void printMatches(char matches[][PATH_MAX],
                         int matchCount)
{
    printf("\n");

    for (int i = 0; i < matchCount; i++)
        printf("%s  ", matches[i]);

    printf("\n");
}

void autocomplete(char* buffer,
                  int& length,
                  const char* homeDirectory)
{
    if (length == 0)
        return;

    int start = length - 1;

    while (start >= 0 &&
           buffer[start] != ' ' &&
           buffer[start] != '\t')
    {
        start--;
    }

    start++;

    char prefix[PATH_MAX];

    int prefixLength = length - start;

    if (prefixLength >= PATH_MAX)
        return;

    strncpy(prefix,
            buffer + start,
            prefixLength);

    prefix[prefixLength] = '\0';

    bool commandMode = true;

    for (int i = 0; i < start; i++)
    {
        if (buffer[i] != ' ' &&
            buffer[i] != '\t')
        {
            commandMode = false;
            break;
        }
    }

    char matches[100][PATH_MAX];
    int matchCount = 0;

    if (commandMode)
    {
        const char* builtins[] = {
            "cd", "echo", "pwd", "ls", "pinfo", "search", "history", "exit"
        };
        int builtinCount = 8;

        for (int i = 0; i < builtinCount; i++)
        {
            if (strncmp(builtins[i], prefix, strlen(prefix)) == 0)
            {
                strcpy(matches[matchCount++], builtins[i]);
            }
        }

        if (matchCount == 0 || strlen(prefix) == 1)
        {
            char* path = getenv("PATH");

            if (path != nullptr)
            {
                char pathCopy[4096];

                strncpy(pathCopy,
                        path,
                        sizeof(pathCopy) - 1);

                pathCopy[sizeof(pathCopy) - 1] = '\0';

                char* directory = strtok(pathCopy, ":");

                while (directory != nullptr &&
                       matchCount < 100)
                {
                    DIR* dir = opendir(directory);

                    if (dir != nullptr)
                    {
                        struct dirent* entry;

                        while ((entry = readdir(dir)) != nullptr &&
                               matchCount < 100)
                        {
                            if (strncmp(entry->d_name,
                                        prefix,
                                        strlen(prefix)) != 0)
                            {
                                continue;
                            }

                            char fullPath[PATH_MAX];

                            snprintf(fullPath,
                                     sizeof(fullPath),
                                     "%s/%s",
                                     directory,
                                     entry->d_name);

                            if (access(fullPath, X_OK) != 0)
                                continue;

                            bool duplicate = false;

                            for (int i = 0; i < matchCount; i++)
                            {
                                if (strcmp(matches[i],
                                           entry->d_name) == 0)
                                {
                                    duplicate = true;
                                    break;
                                }
                            }

                            if (!duplicate)
                            {
                                strcpy(matches[matchCount],
                                       entry->d_name);

                                matchCount++;
                            }
                        }

                        closedir(dir);
                    }

                    directory = strtok(nullptr, ":");
                }
            }
        }
    }
    else
    {
        DIR* dir = opendir(".");

        if (dir == nullptr)
            return;

        struct dirent* entry;

        while ((entry = readdir(dir)) != nullptr &&
               matchCount < 100)
        {
            if (strncmp(entry->d_name,
                        prefix,
                        strlen(prefix)) == 0)
            {
                strcpy(matches[matchCount],
                       entry->d_name);

                matchCount++;
            }
        }

        closedir(dir);
    }

    if (matchCount == 0)
        return;

    if (matchCount == 1)
    {
        buffer[start] = '\0';

        strcat(buffer, matches[0]);

        if (commandMode)
        {
            strcat(buffer, " ");
        }

        length = strlen(buffer);

        redrawInput(buffer, homeDirectory);

        return;
    }

    char commonPrefix[PATH_MAX];

    findCommonPrefix(matches,
                     matchCount,
                     commonPrefix);

    if (strlen(commonPrefix) > strlen(prefix))
    {
        buffer[start] = '\0';

        strcat(buffer, commonPrefix);

        length = strlen(buffer);
    }

    redrawInput(buffer, homeDirectory);

    printMatches(matches, matchCount);

    redrawInput(buffer, homeDirectory);
}