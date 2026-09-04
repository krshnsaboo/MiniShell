#include "shell.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

void executeCommand(char* command, const char* homeDirectory)
{
    char* args[100];

    int argc = parseArguments(command, args);

    if (argc == 0)
        return;

    bool background = false;

    if (strcmp(args[argc - 1], "&") == 0)
    {
        background = true;
        args[--argc] = nullptr;
    }

    if (argc == 0)
        return;

    if (background)
    {
        if (strcmp(args[0], "cd") == 0 ||
            strcmp(args[0], "pwd") == 0 ||
            strcmp(args[0], "echo") == 0 ||
            strcmp(args[0], "ls") == 0 ||
            strcmp(args[0], "pinfo") == 0 ||
            strcmp(args[0], "search") == 0 ||
            strcmp(args[0], "history") == 0)
        {
            printf("Background execution not supported for built-in commands\n");
            return;
        }
    }

    args[argc] = nullptr;

    bool hasRedirection = false;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "<") == 0 ||
            strcmp(args[i], ">") == 0 ||
            strcmp(args[i], ">>") == 0)
        {
            hasRedirection = true;
            break;
        }
    }

    if (strcmp(args[0], "pwd") == 0)
    {
        if (hasRedirection)
            saveStandardDescriptors();

        handleRedirection(args, argc);

        executePwd();

        if (hasRedirection)
        {
            fflush(stdout);
            fflush(stderr);
            restoreStandardDescriptors();
        }

        return;
    }

    if (strcmp(args[0], "echo") == 0)
    {
        if (hasRedirection)
            saveStandardDescriptors();

        handleRedirection(args, argc);

        executeEcho(args, argc);

        if (hasRedirection)
        {
            fflush(stdout);
            fflush(stderr);
            restoreStandardDescriptors();
        }

        return;
    }

    if (strcmp(args[0], "cd") == 0)
    {
        executeCd(args, argc, homeDirectory);
        return;
    }

    if (strcmp(args[0], "ls") == 0)
    {
        if (hasRedirection)
            saveStandardDescriptors();

        handleRedirection(args, argc);

        executeLs(args, argc);

        if (hasRedirection)
        {
            fflush(stdout);
            fflush(stderr);
            restoreStandardDescriptors();
        }

        return;
    }

    if (strcmp(args[0], "pinfo") == 0)
    {
        if (hasRedirection)
            saveStandardDescriptors();

        handleRedirection(args, argc);

        executePinfo(args, argc);

        if (hasRedirection)
        {
            fflush(stdout);
            fflush(stderr);
            restoreStandardDescriptors();
        }

        return;
    }

    if (strcmp(args[0], "search") == 0)
    {
        if (hasRedirection)
            saveStandardDescriptors();

        handleRedirection(args, argc);

        executeSearch(args, argc);

        if (hasRedirection)
        {
            fflush(stdout);
            fflush(stderr);
            restoreStandardDescriptors();
        }

        return;
    }

    if (strcmp(args[0], "history") == 0)
    {
        if (hasRedirection)
            saveStandardDescriptors();

        handleRedirection(args, argc);

        executeHistory(args, argc);

        if (hasRedirection)
        {
            fflush(stdout);
            fflush(stderr);
            restoreStandardDescriptors();
        }

        return;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return;
    }

    if (pid == 0)
    {
        setpgid(0, 0);
        if (hasRedirection)
        {
            handleRedirection(args, argc);
        }

        execvp(args[0], args);

        perror("execvp");
        _exit(1);
    }

    if (background)
    {
        printf("[%d] %s\n", pid, args[0]);

        if (backgroundCount < 100)
        {
            backgroundProcesses[backgroundCount].pid = pid;

            snprintf(
                backgroundProcesses[backgroundCount].command,
                sizeof(backgroundProcesses[backgroundCount].command),
                "%s",
                args[0]
            );

            backgroundCount++;
        }
    }
    else
    {
        foregroundPgid = pid;

        if (setpgid(pid, pid) == -1)
        {
            perror("setpgid");
        }

        int status;

        waitpid(pid, &status, WUNTRACED);

        if (WIFSTOPPED(status))
        {
            printf("\n[%d] stopped\n", pid);
        }

        foregroundPgid = -1;
    }
}
