#include "shell.h"

#include <cstdio>
#include <cstring>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void handleChildExit(int signal)
{
    (void)signal;

    int status;

    for (int i = 0; i < backgroundCount; )
    {
        pid_t pid = waitpid(
            backgroundProcesses[i].pid,
            &status,
            WNOHANG
        );

        if (pid == backgroundProcesses[i].pid)
        {
            printf(
                "\n%s with pid %d exited\n",
                backgroundProcesses[i].command,
                pid
            );

            for (int j = i; j < backgroundCount - 1; j++)
            {
                backgroundProcesses[j] =
                    backgroundProcesses[j + 1];
            }

            backgroundCount--;
        }
        else
        {
            i++;
        }
    }
}

void handleSigInt(int signal)
{
    (void)signal;

    if (foregroundPgid > 0)
    {
        kill(-foregroundPgid, SIGINT);
    }
}

void handleSigTstp(int signal)
{
    (void)signal;

    if (foregroundPgid > 0)
    {
        kill(-foregroundPgid, SIGTSTP);
    }
}