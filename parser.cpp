#include "shell.h"

#include <cstring>

int parseArguments(char* command, char* args[])
{
    int argc = 0;

    char* token = strtok(command, " \t");

    while (token != nullptr && argc < 99)
    {
        args[argc++] = token;
        token = strtok(nullptr, " \t");
    }

    args[argc] = nullptr;

    return argc;
}