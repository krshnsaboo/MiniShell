#include "shell.h"
#include <cstdio>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>

BackgroundProcess backgroundProcesses[100];
int backgroundCount = 0;

char previousDirectory[PATH_MAX] = "";

int savedStdin = -1;
int savedStdout = -1;

pid_t foregroundPgid = -1;

char* findSeparator(char* command, bool& isBackground)
{
    bool insideQuotes = false;

    for (int i = 0; command[i] != '\0'; i++)
    {
        if (command[i] == '"')
        {
            insideQuotes = !insideQuotes;
        }
        else if (!insideQuotes && command[i] == ';')
        {
            isBackground = false;
            return &command[i];
        }
        else if (!insideQuotes && command[i] == '&')
        {
            isBackground = true;
            return &command[i];
        }
    }

    return nullptr;
}

int main() {

    signal(SIGCHLD, handleChildExit);
    signal(SIGINT, handleSigInt);
    signal(SIGTSTP, handleSigTstp);

    char homeDirectory[PATH_MAX];

    if (getcwd(homeDirectory, sizeof(homeDirectory)) == NULL) {
        perror("getcwd");
        return 1;
    }

    initializeHistory(homeDirectory);
    
    while (true) {
        displayPrompt(homeDirectory);

        char command[4096];

        if (readCommandWithHistory(command, sizeof(command), homeDirectory) < 0) {
            break;
        }

        addToHistory(command);

        if (strcmp(command, "exit") == 0)
        {
            break;
        }
        
        char* start = command;

        while (true)
        {
            bool background = false;
            char* separator = findSeparator(start, background);
            if (separator == nullptr)
            {
                if (*start != '\0')
                {
                    if (hasPipe(start))
                        executePipeline(start, homeDirectory);
                    else
                        executeCommand(start, homeDirectory);
                }

                break;
            }

            *separator = '\0';

            if (background)
            {
                char backgroundCommand[4100];

                snprintf(
                    backgroundCommand,
                    sizeof(backgroundCommand),
                    "%s &",
                    start
                );

                if (hasPipe(start))
                    executePipeline(backgroundCommand, homeDirectory);
                else
                    executeCommand(backgroundCommand, homeDirectory);
            }
            else
            {
                if (*start != '\0')
                {
                    if (hasPipe(start))
                        executePipeline(start, homeDirectory);
                    else
                        executeCommand(start, homeDirectory);
                }
            }
            start = separator + 1;
        }
    }

    return 0;
}