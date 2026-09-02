#include "shell.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <termios.h>

static char historyFile[PATH_MAX];

static char historyCommands[20][1024];
static int historyCount = 0;

void initializeHistory(const char* homeDirectory)
{
    snprintf(historyFile,
             sizeof(historyFile),
             "%s/.shell_history",
             homeDirectory);

    FILE* file = fopen(historyFile, "r");

    if (file == nullptr)
        return;

    historyCount = 0;

    while (historyCount < 20 &&
           fgets(historyCommands[historyCount],
                 sizeof(historyCommands[historyCount]),
                 file) != nullptr)
    {
        historyCommands[historyCount][strcspn(
            historyCommands[historyCount], "\n")] = '\0';

        if (historyCommands[historyCount][0] != '\0')
            historyCount++;
    }

    fclose(file);
}

void addToHistory(const char* command)
{
    if (command == nullptr || command[0] == '\0')
        return;

    if (historyCount > 0 &&
        strcmp(historyCommands[historyCount - 1], command) == 0)
    {
        return;
    }

    if (historyCount == 20)
    {
        for (int i = 1; i < 20; i++)
        {
            strcpy(historyCommands[i - 1],
                   historyCommands[i]);
        }

        historyCount--;
    }

    strncpy(historyCommands[historyCount],
            command,
            sizeof(historyCommands[historyCount]) - 1);

    historyCommands[historyCount]
        [sizeof(historyCommands[historyCount]) - 1] = '\0';

    historyCount++;

    FILE* file = fopen(historyFile, "w");

    if (file == nullptr)
        return;

    for (int i = 0; i < historyCount; i++)
        fprintf(file, "%s\n", historyCommands[i]);

    fclose(file);
}

int readCommandWithHistory(char* buffer, int size, const char* homeDirectory)
{
    struct termios oldTermios;
    struct termios newTermios;

    tcgetattr(STDIN_FILENO, &oldTermios);

    newTermios = oldTermios;

    newTermios.c_lflag &= ~(ICANON | ECHO);

    newTermios.c_cc[VMIN] = 1;
    newTermios.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);

    int length = 0;
    int historyIndex = historyCount;

    buffer[0] = '\0';

    while (true)
    {
        char ch;

        if (read(STDIN_FILENO, &ch, 1) <= 0)
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
            return -1;
        }

        if(ch==4){
            tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
            printf("\n");
            fflush(stdout);
            return -1;
        }

        if (ch == '\n' || ch == '\r')
        {
            buffer[length] = '\0';

            printf("\n");
            fflush(stdout);

            tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);

            return length;
        }

        if (ch == 127 || ch == '\b')
        {
            if (length > 0)
            {
                length--;
                buffer[length] = '\0';

                printf("\b \b");
                fflush(stdout);
            }

            continue;
        }

        if (ch == 27)
        {
            char sequence[2];

            if (read(STDIN_FILENO, &sequence[0], 1) <= 0)
                continue;

            if (sequence[0] != '[')
                continue;

            if (read(STDIN_FILENO, &sequence[1], 1) <= 0)
                continue;

            if (sequence[1] == 'A')
            {
                if (historyCount == 0)
                    continue;

                if (historyIndex > 0)
                    historyIndex--;

                strcpy(buffer, historyCommands[historyIndex]);
                length = strlen(buffer);

                printf("\r\033[K");
                displayPrompt(
                    homeDirectory
                );
                printf("%s", buffer);

                fflush(stdout);
            }

            else if (sequence[1] == 'B')
            {
                if (historyIndex < historyCount - 1)
                {
                    historyIndex++;

                    strcpy(
                        buffer,
                        historyCommands[historyIndex]
                    );

                    length = strlen(buffer);
                }
                else
                {
                    historyIndex = historyCount;

                    buffer[0] = '\0';
                    length = 0;
                }

                printf("\r\033[K");
                displayPrompt(
                    homeDirectory
                );
                printf("%s", buffer);

                fflush(stdout);
            }

            continue;
        }

        if (ch == '\t')
        {
            autocomplete(buffer, length, homeDirectory);
            continue;
        }

        if (length < size - 1)
        {
            buffer[length++] = ch;
            buffer[length] = '\0';

            putchar(ch);
            fflush(stdout);
        }
    }
}

void executeHistory(char* args[], int argc)
{
    if (argc > 2)
    {
        printf("Invalid arguments\n");
        return;
    }

    int num = 10;

    if (argc == 2)
    {
        num = atoi(args[1]);

        if (num <= 0 || num >= 20)
        {
            printf("Invalid arguments\n");
            return;
        }
    }

    int start = historyCount - num;

    if (start < 0)
        start = 0;

    for (int i = start; i < historyCount; i++)
        printf("%s\n", historyCommands[i]);
}