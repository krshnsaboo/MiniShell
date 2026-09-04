#include "shell.h"
#include <iostream>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <grp.h>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <unistd.h>

static bool expandHomePath(const char* input,
                          const char* homeDirectory,
                          char* output,
                          size_t outputSize)
{
    if (strcmp(input, "~") == 0)
    {
        snprintf(output, outputSize, "%s", homeDirectory);
        return true;
    }

    if (strncmp(input, "~/", 2) == 0)
    {
        snprintf(output, outputSize, "%s/%s", homeDirectory, input + 2);
        return true;
    }

    return false;
}

void executeCd(char* args[], int argc, const char* homeDirectory)
{
    if (argc > 2)
    {
        printf("Invalid arguments\n");
        return;
    }

    char currentDirectory[PATH_MAX];

    if (getcwd(currentDirectory, sizeof(currentDirectory)) == NULL)
    {
        perror("getcwd");
        return;
    }

    if (argc == 1)
    {
        strcpy(previousDirectory, currentDirectory);

        if (chdir(homeDirectory) != 0)
            perror("cd");

        return;
    }

    if (strcmp(args[1], "-") == 0)
    {
        if (previousDirectory[0] == '\0')
        {
            printf("OLDPWD not set\n");
            return;
        }

        char temp[PATH_MAX];
        strcpy(temp, previousDirectory);

        if (chdir(temp) != 0)
        {
            perror("cd");
            return;
        }

        strcpy(previousDirectory, currentDirectory);

        printf("%s\n", temp);
        return;
    }

    char targetPath[PATH_MAX];
    const char* target = args[1];

    if (expandHomePath(args[1], homeDirectory, targetPath, sizeof(targetPath)))
        target = targetPath;

    strcpy(previousDirectory, currentDirectory);

    if (chdir(target) != 0)
        perror("cd");
}

void executeEcho(char* args[], int argc)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s", args[i]);

        if (i != argc - 1)
            printf(" ");
    }

    printf("\n");
}

void executePwd()
{
    char currentDirectory[PATH_MAX];

    if (getcwd(currentDirectory, sizeof(currentDirectory)) == NULL)
    {
        perror("pwd");
        return;
    }

    printf("%s\n", currentDirectory);
}

void executePinfo(char* args[], int argc)
{
    pid_t pid;

    if (argc == 1)
    {
        pid = getpid();
    }
    else if (argc == 2)
    {
        pid = atoi(args[1]);
    }
    else
    {
        printf("Invalid arguments\n");
        return;
    }

    printf("pid -- %d\n", pid);

    char path[PATH_MAX];
    char buffer[1024];

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE* file = fopen(path, "r");

    if (file == NULL)
    {
        perror("pinfo");
        return;
    }

    if (fgets(buffer, sizeof(buffer), file) == NULL)
    {
        fclose(file);
        perror("pinfo");
        return;
    }

    fclose(file);

    char state;
    unsigned long memory;

    char* ptr = strrchr(buffer, ')');

    if (ptr == NULL)
        return;

    sscanf(ptr + 2, "%c", &state);

    char statusString[3];

    statusString[0] = state;
    statusString[1] = '\0';

    if (pid == getpid())
    {
        statusString[1] = '+';
        statusString[2] = '\0';
    }

    printf("process Status -- %s\n", statusString);

    snprintf(path, sizeof(path), "/proc/%d/statm", pid);

    file = fopen(path, "r");

    if (file != NULL)
    {
        unsigned long pages;

        if (fscanf(file, "%lu", &pages) == 1)
        {
            memory = pages * sysconf(_SC_PAGESIZE);
            printf("memory -- %lu {Virtual Memory}\n", memory);
        }

        fclose(file);
    }

    snprintf(path, sizeof(path), "/proc/%d/exe", pid);

    char executablePath[PATH_MAX];

    ssize_t len = readlink(path, executablePath, sizeof(executablePath) - 1);

    if (len != -1)
    {
        executablePath[len] = '\0';

        printf("executable Path -- %s\n", executablePath);
    }
}

void printPermissions(mode_t mode)
{
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void printLongEntry(const char* directory, const char* name)
{
    char path[PATH_MAX];

    snprintf(path, sizeof(path), "%s/%s", directory, name);

    struct stat fileInfo;

    if (stat(path, &fileInfo) != 0)
    {
        perror("ls");
        return;
    }

    printPermissions(fileInfo.st_mode);

    printf(" %lu", (unsigned long)fileInfo.st_nlink);

    struct passwd* owner = getpwuid(fileInfo.st_uid);
    struct group* group = getgrgid(fileInfo.st_gid);

    printf(" %s", owner ? owner->pw_name : "unknown");
    printf(" %s", group ? group->gr_name : "unknown");

    printf(" %lld", (long long)fileInfo.st_size);

    char timeBuffer[80];

    struct tm* timeInfo = localtime(&fileInfo.st_mtime);

    strftime(timeBuffer, sizeof(timeBuffer), "%b %d %H:%M", timeInfo);

    printf(" %s %s\n", timeBuffer, name);
}

void listDirectory(const char* directory, bool showHidden, bool longFormat)
{
    DIR* dir = opendir(directory);

    if (dir == NULL)
    {
        perror("ls");
        return;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!showHidden && entry->d_name[0] == '.')
            continue;

        if (longFormat)
            printLongEntry(directory, entry->d_name);
        else
            printf("%s\n", entry->d_name);
    }

    closedir(dir);
}

void executeLs(char* args[], int argc)
{
    bool showHidden = false;
    bool longFormat = false;

    char* paths[100];
    int pathCount = 0;

    for (int i = 1; i < argc; i++)
    {
        if (args[i][0] == '-')
        {
            for (int j = 1; args[i][j] != '\0'; j++)
            {
                if (args[i][j] == 'a')
                    showHidden = true;
                else if (args[i][j] == 'l')
                    longFormat = true;
                else
                {
                    printf("Invalid arguments\n");
                    return;
                }
            }
        }
        else
        {
            paths[pathCount++] = args[i];
        }
    }

    if (pathCount == 0)
    {
        paths[pathCount++] = (char*)".";
    }

    for (int i = 0; i < pathCount; i++)
    {
        char resolvedPath[PATH_MAX];
        const char* target = paths[i];

        if (expandHomePath(paths[i], getenv("HOME"), resolvedPath, sizeof(resolvedPath)))
            target = resolvedPath;

        if (pathCount > 1)
            printf("%s:\n", target);

        struct stat info;

        if (stat(target, &info) != 0)
        {
            perror("ls");
            continue;
        }

        if (S_ISDIR(info.st_mode))
            listDirectory(target, showHidden, longFormat);
        else
        {
            if (longFormat)
                printLongEntry(".", target);
            else
                printf("%s\n", target);
        }

        if (i != pathCount - 1)
            printf("\n");
    }
}

void searchRecursive(const char* directory, const char* target, bool& found)
{
    DIR* dir = opendir(directory);

    if (dir == NULL)
        return;

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];

        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        if (strcmp(entry->d_name, target) == 0)
            found = true;

        struct stat info;

        if (stat(path, &info) == 0 && S_ISDIR(info.st_mode))
            searchRecursive(path, target, found);
    }

    closedir(dir);
}

void executeSearch(char* args[], int argc)
{
    if (argc != 2)
    {
        printf("Invalid arguments\n");
        return;
    }

    bool found = false;

    searchRecursive(".", args[1], found);

    printf(found ? "True\n" : "False\n");
}
