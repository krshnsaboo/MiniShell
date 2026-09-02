#include "shell.h"

#include <iostream>
#include <pwd.h>
#include <sys/utsname.h>
#include <cstring>

void displayPrompt(const char* homeDirectory)
{
    char currentDirectory[PATH_MAX];

    if (getcwd(currentDirectory, sizeof(currentDirectory)) == NULL)
    {
        perror("getcwd");
        return;
    }

    struct passwd* pw = getpwuid(getuid());

    struct utsname systemInfo;

    if (uname(&systemInfo) == -1)
    {
        perror("uname");
        return;
    }

    const char* username = pw->pw_name;
    const char* hostname = systemInfo.nodename;

    if (strcmp(currentDirectory, homeDirectory) == 0)
    {
        printf("<%s@%s:~>", username, hostname);
    }
    else if (strncmp(currentDirectory,
                     homeDirectory,
                     strlen(homeDirectory)) == 0)
    {
        printf("<%s@%s:~%s>",
               username,
               hostname,
               currentDirectory + strlen(homeDirectory));
    }
    else
    {
        printf("<%s@%s:%s>",
               username,
               hostname,
               currentDirectory);
    }

    fflush(stdout);
}