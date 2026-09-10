#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>

struct BackgroundProcess
{
    pid_t pid;
    char command[256];
};

extern BackgroundProcess backgroundProcesses[100];
extern int backgroundCount;

extern char previousDirectory[PATH_MAX];

extern int savedStdin;
extern int savedStdout;

extern pid_t foregroundPgid;

void displayPrompt(const char* homeDirectory);
int parseArguments(char* command, char* args[]);
void executeCd(char* args[], int argc, const char* homeDirectory);
void executeEcho(char* args[], int argc);
void executePwd();
void executePinfo(char* args[], int argc);
void executeLs(char* args[], int argc, const char* homeDirectory);
void executeSearch(char* args[], int argc);
void printPermissions(mode_t mode);
void printLongEntry(const char* directory, const char* name);
void listDirectory(const char* directory, bool showHidden, bool longFormat);
void searchRecursive(const char* directory, const char* target, bool& found);
bool handleRedirection(char* args[], int& argc);
void saveStandardDescriptors();
void restoreStandardDescriptors();
bool hasPipe(char* command);
void executePipeline(char* command, const char* homeDirectory);
void executeCommand(char* command, const char* homeDirectory);
void handleChildExit(int signal);
void handleSigInt(int signal);
void handleSigTstp(int signal);
void executeHistory(char* args[], int argc);
void initializeHistory(const char* homeDirectory);
void addToHistory(const char* command);
int readCommandWithHistory(char* buffer, int size, const char* homeDirectory);
void autocomplete(char* buffer, int& length, const char* homeDirectory);

#endif