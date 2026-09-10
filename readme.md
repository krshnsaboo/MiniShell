POSIX-like Shell
================

Overview
--------
This project is a modular POSIX-like interactive shell implemented in C++.
The shell supports common builtin commands (cd, echo, pwd, ls, pinfo, search, history), pipelines, input/output redirection (<, >, >>), background (&) and foreground processes, simple job handling, autocomplete and a small persistent command history.

Design & Flow (high level)
--------------------------
- main.cpp
  - Entry point. Initializes history, registers signal handlers, and runs the main read-execute loop.
  - Reads user input (with line-editing, history navigation and autocomplete) and splits semi-colon separated command lists and background markers.

- prompt.cpp
  - Builds and prints the shell prompt in the form: <username@hostname:cwd>
  - Displays ~ for the shell's home directory.

- history.cpp
  - Persists the last 20 commands to ~/.shell_history and provides up/down navigation and the history builtin.

- autocomplete.cpp
  - Implements TAB completion for both executable commands (searching $PATH) and local files/directories.

- parser.cpp / execution.cpp
  - parser.cpp performs basic tokenization where needed; execution.cpp contains the high-level logic to dispatch commands.
  - Builtin detection: builtins are handled directly (no execvp) so stateful operations (cd, history) affect the shell.
  - For non-builtins, the shell forks and execvp()s the requested program.
  - Background commands (ending with &) are launched without waiting; the PID is printed and background jobs tracked.

- builtins.cpp
  - Implementations for cd (with ~, -, ., .. handling), echo, pwd, ls (flags -a, -l and multiple paths), pinfo, search (recursive), and history output.

- pipeline.cpp
  - Implements pipelines of arbitrary length using pipes and multiple children with carefully set file descriptors and process groups.

- redirection.cpp
  - Handles input (<), output overwrite (>), and output append (>>) by duplicating file descriptors and removing redirection tokens from argument lists.
  - The shell saves and restores standard descriptors when builtins perform redirected output.

- signals.cpp
  - Handles SIGCHLD (reaping background processes and notifying on exit), SIGINT (forwarding Ctrl-C to foreground process group), and SIGTSTP (Ctrl-Z forwarding) so the shell remains responsive.

Files & Structure
-----------------
- main.cpp           — program entry and main control loop
- prompt.cpp         — prompt construction and printing
- parser.cpp         — tokenization utilities
- execution.cpp      — command dispatch, builtin handling, external exec
- builtins.cpp       — implementations for cd, ls, echo, pwd, pinfo, search, history
- pipeline.cpp       — pipeline execution
- redirection.cpp    — input/output redirection helpers
- history.cpp        — persistent history and line-edit handling
- autocomplete.cpp   — tab completion implementation
- signals.cpp        — signal handlers
- shell.h            — shared declarations and constants
- Makefile           — build rules (g++ -std=c++17)
- shell_test_fixtures— sample files used while testing the shell

Build
-----
Run from the repository root:

    make

This produces the executable ./shell

Run
---
Start the shell by running:

    ./shell

Then interact using normal shell syntax. Supported features include:
- Semicolon-separated command lists: cmd1 ; cmd2
- Background execution: cmd &
- Pipelines: cmd1 | cmd2 | cmd3
- Redirection: cmd > out.txt, cmd >> out.txt, cmd < in.txt
- Builtins: cd, echo, pwd, ls, pinfo, search, history
- Autocomplete (TAB) and history navigation (Up/Down arrows)