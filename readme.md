
# Advanced Operating Systems — Assignment 2

## POSIX Shell Implementation

**Roll Number:** 2026201025  
**Course:** CS3.304 — Advanced Operating Systems  
**Assignment:** 2

This project implements a modular Unix-like command-line shell in C++ using Linux/POSIX system calls and APIs.

## Features

### 1. Dynamic Prompt

The shell displays:

text
<username@system_name:current_directory>


The home/invocation directory is represented as `~`.

### 2. Built-in Commands

The following commands are implemented internally without using `execvp`:

* `cd`
* `pwd`
* `echo`
* `ls`
* `pinfo`
* `search`
* `history`

Supported `cd` functionality includes:

* `cd`
* `cd ~`
* `cd .`
* `cd ..`
* `cd -`

`ls` supports:

* `-a`
* `-l`
* `-al`
* `-la`
* Multiple files/directories and flags

### 3. External Commands

External Linux commands are executed using:

```text
fork()
execvp()
waitpid()
```

Both foreground and background execution are supported.

Example:

```text
sleep 5
sleep 5 &
```

Background processes are tracked and their termination is reported by the shell.

### 4. Command Sequencing

Multiple commands can be executed using `;`.

Example:

```text
pwd ; ls ; echo hello
```

### 5. I/O Redirection

The shell supports:

```text
<       Input redirection
>       Output redirection
>>      Output append
```

Examples:

```text
cat < input.txt
echo hello > output.txt
echo world >> output.txt
```

Output files are created with permission mode `0644`.

### 6. Pipes

Arbitrary-length pipelines are supported.

Examples:

```text
cat file.txt | grep hello
```

```text
cat file.txt | grep hello | wc -l
```

Pipelines can also be executed in the background.

### 7. Process Information

`pinfo` obtains process information through the Linux `/proc` filesystem.

Example:

```text
pinfo
pinfo 1234
```

The shell displays:

* Process ID
* Process status
* Virtual memory
* Executable path

### 8. Recursive Search

The `search` command recursively searches the current directory.

Example:

```text
search shell.h
```

It prints whether the requested file/directory exists.

### 9. Signal Handling

The shell handles:

* `Ctrl-C` — interrupts the foreground process
* `Ctrl-Z` — stops the foreground process and returns control to the shell
* `Ctrl-D` — exits the shell

Process groups are used so that signals are delivered correctly to foreground pipelines.

### 10. Command History

The shell maintains persistent command history.

Features include:

* Maximum 20 stored commands
* Maximum 10 commands displayed by `history`
* `history <num>` support
* Persistent history across shell sessions
* Duplicate consecutive commands are avoided
* Up/Down arrow navigation

Examples:

```text
history
history 5
```

### 11. TAB Autocomplete

TAB completion is supported for:

* Commands available through `PATH`
* Files
* Directories

The shell supports:

* Single-match completion
* Multiple-match suggestions
* Common-prefix completion

Example:

```text
ech<TAB>
```

can complete to:

```text
echo
```

## Project Structure

```text
2026201025_Assignment2/
│
├── main.cpp
├── shell.h
├── prompt.cpp
├── parser.cpp
├── builtins.cpp
├── execution.cpp
├── redirection.cpp
├── pipeline.cpp
├── signals.cpp
├── history.cpp
├── autocomplete.cpp
├── Makefile
└── readme.md
```

### File Responsibilities

| File               | Responsibility                              |
| ------------------ | ------------------------------------------- |
| `main.cpp`         | Shell initialization and main command loop  |
| `shell.h`          | Shared declarations and data structures     |
| `prompt.cpp`       | Dynamic shell prompt                        |
| `parser.cpp`       | Command argument parsing                    |
| `builtins.cpp`     | Built-in commands                           |
| `execution.cpp`    | Foreground/background command execution     |
| `redirection.cpp`  | Input/output redirection                    |
| `pipeline.cpp`     | Multi-stage pipeline execution              |
| `signals.cpp`      | SIGINT, SIGTSTP and SIGCHLD handling        |
| `history.cpp`      | Persistent history and arrow-key navigation |
| `autocomplete.cpp` | TAB command/file completion                 |
| `Makefile`         | Compilation                                 |

## Compilation

### Requirements

* Linux
* `g++`
* POSIX/Linux system-call support

Compile using:

```bash
make
```

Clean compiled files using:

```bash
make clean
```

## Running the Shell

After compilation:

```bash
./shell
```

The shell starts in the directory from which it is invoked.

## System Calls / APIs Used

The implementation makes use of Linux/POSIX facilities including:

```text
fork()
execvp()
waitpid()
pipe()
dup()
dup2()
open()
close()
read()
write()
kill()
setpgid()
getcwd()
chdir()
opendir()
readdir()
stat()
readlink()
signal()
termios
/proc filesystem
```

## Restrictions Followed

The implementation does not use:

* `system()`
* `popen()`
* `pclose()`
* `ncurses`
* C++17 filesystem APIs
* `execvp()` for built-in commands

The implementation uses modular C++ source files rather than placing the complete shell implementation in a single file.

## Build and Test

Build:

```bash
make clean
make
```

Run:

```bash
./shell
```

The shell was tested for built-in commands, external commands, redirection, pipelines, background execution, process information, signals, history, and TAB autocomplete.
