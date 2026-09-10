#include "shell.h"

int parseArguments(char* command, char* args[])
{
    int argc = 0;
    int r = 0;
    int w = 0;

    while (command[r] != '\0' && argc < 99)
    {
        while (command[r] == ' ' || command[r] == '\t')
            r++;

        if (command[r] == '\0')
            break;

        char* tokenStart = &command[w];
        bool hasContent = false;

        while (command[r] != '\0' && command[r] != ' ' && command[r] != '\t')
        {
            if (command[r] == '"' || command[r] == '\'')
            {
                char quote = command[r++];
                hasContent = true;

                while (command[r] != '\0' && command[r] != quote)
                {
                    command[w++] = command[r++];
                }

                if (command[r] == quote)
                {
                    r++;
                }
            }
            else
            {
                hasContent = true;
                command[w++] = command[r++];
            }
        }

        bool atEnd = (command[r] == '\0');
        if (!atEnd)
            r++;

        command[w++] = '\0';

        if (hasContent || tokenStart == &command[w - 1])
        {
            args[argc++] = tokenStart;
        }

        if (atEnd)
            break;
    }

    args[argc] = nullptr;
    return argc;
}