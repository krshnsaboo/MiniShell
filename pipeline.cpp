#include "shell.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

bool hasPipe(char* command)
{
    return strchr(command, '|') != nullptr;
}

bool background = false;

void executePipeline(char* command, const char* /*homeDirectory*/)
{
    char* commands[100];
    int commandCount = 0;

    char* token = strtok(command, "|");

    while (token != nullptr && commandCount < 100)
    {
        commands[commandCount++] = token;
        token = strtok(nullptr, "|");
    }

    bool background = false;

    char* lastCommand = commands[commandCount - 1];

    int length = strlen(lastCommand);

    while(length > 0 && (lastCommand[length - 1] == ' ' || lastCommand[length - 1] == '\t'))
    {
        lastCommand[length - 1] = '\0';
        length--;
    }

    if (length > 0 && lastCommand[length - 1] == '&')
    {
        background = true;
        lastCommand[length - 1] = '\0';
    }

    if (commandCount == 0)
        return;

    int pipefds[2 * (commandCount - 1)];

    for (int i = 0; i < commandCount - 1; i++)
    {
        if (pipe(pipefds + 2 * i) < 0)
        {
            perror("pipe");
            return;
        }
    }

    pid_t pids[100];

    for (int i = 0; i < commandCount; i++)
    {
        pids[i] = fork();

        if (pids[i] < 0)
        {
            perror("fork");
            return;
        }

        if (pids[i] == 0)
        {
            if(i==0)
            {
                setpgid(0, 0);
            }
            else
            {
                setpgid(0, pids[0]);
            }

            if (i > 0)
            {
                dup2(pipefds[2 * (i - 1)], STDIN_FILENO);
            }

            if (i < commandCount - 1)
            {
                dup2(pipefds[2 * i + 1], STDOUT_FILENO);
            }

            for (int j = 0; j < 2 * (commandCount - 1); j++)
                close(pipefds[j]);

            char* args[100];

            int argc = parseArguments(commands[i], args);

            if (argc == 0)
                _exit(0);

            handleRedirection(args, argc);

            execvp(args[0], args);

            perror("execvp");
            _exit(1);
        }
        else {
            if(i==0)
            {
                setpgid(pids[i], pids[i]);
            }
            else
            {
                setpgid(pids[i], pids[0]);
            }
        }
    }

    foregroundPgid = pids[0];

    for (int i = 0; i < 2 * (commandCount - 1); i++)
        close(pipefds[i]);

    if (!background)
    {
        foregroundPgid = pids[0];
        int status;
        for (int i = 0; i < commandCount; i++)
        {
            waitpid(pids[i], &status, WUNTRACED);
        }

        if (WIFSTOPPED(status))
        {
            printf("\n[%d] stopped\n", pids[0]);
        }
        foregroundPgid = -1;
    }
    else
    {
        printf("[%d] pipeline\n", pids[0]);
    }
}