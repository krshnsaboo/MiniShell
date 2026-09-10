# POSIX Shell: Architecture & Test Case Workflows

This document provides a comprehensive, end-to-end technical explanation of the **POSIX Shell Assignment (Monsoon 2026)**. It is divided into two parts:
1. **Overall Architectural Workflow**: The internal lifecycle of the shell, from startup to command processing, execution, piping, redirection, signal handling, and termination.
2. **Individual Test Case Workflows**: An exhaustive, step-by-step trace of every single test case (1 through 38) from `commands.txt`, detailing the exact system calls, data structures, and kernel-level interactions involved.

---

# Part 1: Overall Shell Architecture & Workflow

```mermaid
flowchart TD
    A[Shell Start: main.cpp] --> B[Register Signal Handlers: SIGCHLD, SIGINT, SIGTSTP]
    B --> C[Determine homeDirectory via getcwd]
    C --> D[Initialize History from .shell_history]
    D --> E[Display Prompt: <username@hostname:cwd>]
    E --> F[readCommandWithHistory: Termios Raw Mode]
    F --> G{Special Key?}
    G -- TAB --> H[autocomplete.cpp: Builtins, PATH, Files] --> F
    G -- UP/DOWN Arrow --> I[history.cpp: History Buffer Traversal] --> F
    G -- Ctrl-D --> J[Exit Shell Cleanly]
    G -- Enter --> K[Save Line to History: addToHistory]
    K --> L[Split Semicolons & Detect Background: findSeparator]
    L --> M{Contains Pipe '|'?}
    M -- Yes --> N[executePipeline: Fork n children, pipe, dup2, exec/builtin]
    M -- No --> O[executeCommand: Parse Arguments & Redirections]
    O --> P{Builtin Command?}
    P -- Yes --> Q[Execute in Parent Process: cd, echo, pwd, ls, pinfo, search, history]
    P -- No --> R[Fork Child Process]
    R --> S{Background &?}
    S -- Yes --> T[Set PGID, Print PID, Shell Continues Without Waiting]
    S -- No --> U[Set foregroundPgid, waitpid WUNTRACED]
    T --> E
    U --> E
    Q --> E
```

---

## 1. Shell Initialization Lifecycle (`main.cpp`)

1. **Signal Registration**:
   * Registers `handleChildExit` for `SIGCHLD` to reap background children asynchronously without blocking or creating zombie processes.
   * Registers `handleSigInt` for `SIGINT` (Ctrl-C) and `handleSigTstp` for `SIGTSTP` (Ctrl-Z) to intercept terminal signals and route them only to the active foreground process group.
2. **Home Directory Anchoring**:
   * Calls `getcwd(homeDirectory, sizeof(homeDirectory))` upon invocation.
   * This directory is permanently treated as the shell's home (`~`). Any subfolder displays as `~/<relative_path>`, and the prompt displays `~` when cwd matches this directory.
3. **Persistent History Initialization**:
   * Reads up to the last 20 commands stored in `<homeDirectory>/.shell_history` into an in-memory ring buffer `historyCommands[20]`.

---

## 2. Interactive Input, Line Editing & Autocomplete (`history.cpp`, `autocomplete.cpp`)

The shell uses low-level POSIX terminal control (`termios`) rather than standard `fgets` or `readline`:
1. **Raw Mode**:
   * Calls `tcgetattr(STDIN_FILENO, &oldTermios)`.
   * Clears `ICANON` (canonical mode) and `ECHO` (terminal echo), allowing the shell to process keystrokes individually in real time.
2. **Character & Sequence Processing**:
   * **Printable Characters**: Appended to the line buffer and echoed to stdout with `putchar()`.
   * **Backspace (`127` or `\b`)**: Removes the last character from buffer and updates terminal with `\b \b`.
   * **Escape Sequences (`\033[...]`)**:
     * `\033[A` (**UP Arrow**): Traverses backward through `historyCommands`, clears the terminal line with `\r\033[K`, reprints the prompt, and populates the buffer with the previous command.
     * `\033[B` (**DOWN Arrow**): Traverses forward through `historyCommands` toward newer commands or an empty line.
   * **Tab (`\t`) - Autocomplete**:
     * Parses the current word prefix.
     * **Command Mode** (first token on line): Matches against shell builtins (`cd`, `echo`, `pwd`, `ls`, `pinfo`, `search`, `history`, `exit`) and executable files across directories in `$PATH`.
     * If a single match is found: completes the token and appends a space `' '`.
     * If multiple matches exist: finds the longest common prefix, expands the buffer up to that prefix, prints all matches, and redraws the prompt with the input buffer intact.
     * **Argument/File Mode**: Scans the current working directory using `opendir(".")` and `readdir()` to complete filenames and subdirectories.
   * **Ctrl-D (`\x04`)**:
     * If pressed on an empty line, restores old termios settings and returns `-1`, prompting a clean logout.
   * **Enter (`\r` or `\n`)**:
     * Null-terminates the buffer, restores standard terminal settings, prints a newline, and passes the command string to the execution loop.

---

## 3. Command Tokenization & Quote Handling (`parser.cpp`)

1. **Quote-Aware Parsing**:
   * Scans the command string using dual read (`r`) and write (`w`) pointers.
   * Treats single quotes (`'...'`) and double quotes (`"..."`) as literal enclosures.
   * Whitespace inside quotes is preserved verbatim (it is **not** treated as an argument delimiter).
   * Surrounding quote characters are stripped during tokenization so programs receive unquoted argument values.
2. **Argument Array**:
   * Populates `char* args[100]`, terminating with `nullptr`.

---

## 4. Execution & Job Control (`execution.cpp`)

1. **Background Detection**:
   * Checks if the last token is `&`. If present, sets `background = true` and trims `&`.
   * Builtin commands (`cd`, `echo`, `pwd`, `ls`, `pinfo`, `search`, `history`) cannot be run in the background.
2. **Redirection Handling (`redirection.cpp`)**:
   * Inspects arguments for `<`, `>`, and `>>`.
   * Input `<`: opens the file with `O_RDONLY` and redirects standard input using `dup2(fd, STDIN_FILENO)`.
   * Output `>`: opens with `O_WRONLY | O_CREAT | O_TRUNC, 0644` and redirects stdout via `dup2(fd, STDOUT_FILENO)`.
   * Output append `>>`: opens with `O_WRONLY | O_CREAT | O_APPEND, 0644` and redirects stdout.
   * Removes redirection symbols and filenames from the argument array.
3. **Builtin Execution**:
   * If the command is a builtin, standard descriptors (stdin/stdout) are saved using `dup()`.
   * Redirection is applied. If redirection fails, execution is aborted and descriptors restored.
   * The builtin executes directly in the shell process, and descriptors are restored using `dup2()`.
4. **External System Command Execution**:
   * Calls `fork()`.
   * **Child Process**:
     * Calls `setpgid(0, 0)` to place itself in a new process group.
     * Applies I/O redirection; on error, calls `_exit(1)`.
     * Invokes `execvp(args[0], args)`. On failure, outputs `perror("execvp")` and exits.
   * **Parent Process**:
     * **Background (`&`)**: Prints `[<pid>] <command>`, records the process in `backgroundProcesses[100]`, and immediately returns to prompt.
     * **Foreground**: Sets `foregroundPgid = pid`, sets child process group, and waits using `waitpid(pid, &status, WUNTRACED)`.
     * If the child was suspended (via Ctrl-Z), detects `WIFSTOPPED(status)` and outputs `[<pid>] stopped`.

---

## 5. Pipeline Architecture (`pipeline.cpp`)

1. **Splitting Commands**:
   * Splits command string on `|` into `commandCount` sub-commands.
2. **Pipe Array Creation**:
   * Allocates `2 * (commandCount - 1)` file descriptors using `pipe()`.
3. **Child Forking & FD Plumbing**:
   * Forks `commandCount` child processes.
   * First child sets group leader: `setpgid(0, 0)`. Subsequent children join the first child's group: `setpgid(0, pids[0])`.
   * If `i > 0`: `dup2(pipefds[2 * (i - 1)], STDIN_FILENO)` (reads from previous stage).
   * If `i < commandCount - 1`: `dup2(pipefds[2 * i + 1], STDOUT_FILENO)` (writes to next stage).
   * Closes all unneeded pipe descriptors in the child.
   * Applies local redirections (e.g. `< in.txt` or `> out.txt`).
   * **Builtin Support**: Checks if stage is a builtin; if so, runs it directly and exits with `_exit(0)`. Otherwise, executes `execvp`.
4. **Parent Coordination**:
   * Parent closes all pipe descriptors to avoid hanging on open write ends.
   * Forwards terminal focus: `foregroundPgid = pids[0]`.
   * Waits for all children in the pipeline using `waitpid(pids[i], &status, WUNTRACED)`.

---

# Part 2: Detailed Workflow of All 38 Checklist Test Cases

---

### Step 1: `pwd`
* **Input**: `pwd`
* **Workflow**:
  1. `readCommandWithHistory` receives `pwd\n`.
  2. `findSeparator` detects no `;` or `&`.
  3. `parseArguments` produces `args[0] = "pwd"`, `argc = 1`.
  4. `executeCommand` identifies `"pwd"` as a builtin.
  5. `executePwd()` invokes `getcwd(currentDirectory, sizeof(currentDirectory))`.
  6. Outputs the absolute path of the current disposable folder followed by `\n`.
* **Result**: **PASS**

---

### Step 2: `echo "hello" ""world"`
* **Input**: `echo "hello" ""world"`
* **Workflow**:
  1. `parseArguments` parses `"hello"` -> token `hello`.
  2. `""world"` has empty quotes `""` followed by `world` -> token `world`.
  3. `args` array becomes `["echo", "hello", "world"]`.
  4. `executeEcho` iterates through arguments starting at index 1, printing each with a single space delimiter.
  5. Outputs `hello world`.
* **Result**: **PASS**

---

### Step 3: `echo Aos          Assignment          2`
* **Input**: `echo Aos          Assignment          2`
* **Workflow**:
  1. `parseArguments` encounters unquoted words separated by multiple space characters.
  2. The parser skips whitespace runs and produces separate tokens: `args = ["echo", "Aos", "Assignment", "2"]`.
  3. `executeEcho` joins the tokens with single spaces.
  4. Outputs `Aos Assignment 2`.
* **Result**: **PASS**

---

### Step 4: `echo "Aos          Assignment          2"`
* **Input**: `echo "Aos          Assignment          2"`
* **Workflow**:
  1. `parseArguments` encounters opening quote `"`.
  2. It enters quote-handling mode, reading all characters (including multiple spaces) until the matching closing quote.
  3. Produces a single token containing all internal spaces: `args[1] = "Aos          Assignment          2"`.
  4. `executeEcho` prints argument 1 directly.
  5. Outputs `Aos          Assignment          2`.
* **Result**: **PASS**

---

### Step 5: `cd files; pwd; cd ~; cd -`
* **Input**: `cd files; pwd; cd ~; cd -`
* **Workflow**:
  1. `findSeparator` splits commands by `;`.
  2. **`cd files`**: `executeCd` records current directory in `previousDirectory`, calls `chdir("files")`. Working directory changes to `.../files`.
  3. **`pwd`**: `executePwd` calls `getcwd()` and prints `.../files`.
  4. **`cd ~`**: `expandHomePath("~")` expands `~` to `homeDirectory`. `chdir(homeDirectory)` moves back to root of shell. `previousDirectory` updated to `.../files`.
  5. **`cd -`**: Checks `previousDirectory`, changes directory to `.../files`, and prints the target path.
* **Result**: **PASS**

---

### Step 6: `cd .; cd ..; cd; pwd; cd files`
* **Input**: `cd .; cd ..; cd; pwd; cd files`
* **Workflow**:
  1. **`cd .`**: `chdir(".")` leaves directory unchanged.
  2. **`cd ..`**: `chdir("..")` moves up to parent directory.
  3. **`cd`**: Called with `argc == 1`. Default target is `homeDirectory`. Calls `chdir(homeDirectory)`.
  4. **`pwd`**: Prints `homeDirectory`.
  5. **`cd files`**: Enters `files/`.
* **Result**: **PASS**

---

### Step 7: `cd one two; cd no_such_directory; pwd`
* **Input**: `cd one two; cd no_such_directory; pwd`
* **Workflow**:
  1. **`cd one two`**: `parseArguments` yields `argc = 3`. `executeCd` detects `argc > 2`, prints `Invalid arguments`, and aborts without changing directory.
  2. **`cd no_such_directory`**: `chdir("no_such_directory")` returns `-1`. `perror("cd")` outputs `cd: No such file or directory`.
  3. **`pwd`**: Confirms directory remains `.../files`.
* **Result**: **PASS**

---

### Step 8: `ls`
* **Input**: `ls`
* **Workflow**:
  1. `executeLs` defaults to target `"."` with `showHidden = false` and `longFormat = false`.
  2. `opendir(".")` reads directory entries via `readdir()`.
  3. Entries starting with `.` are filtered out.
  4. Prints matching entries (`Test`, `file1.txt`, `in.txt`, `lines.txt`) in single column.
* **Result**: **PASS**

---

### Step 9: `ls -a -l ~ . -al .. lines.txt`
* **Input**: `ls -a -l ~ . -al .. lines.txt`
* **Workflow**:
  1. `executeLs` scans argument flags: `-a`, `-l`, `-al` set `showHidden = true` and `longFormat = true`.
  2. Path list: `~`, `.`, `..`, `lines.txt`.
  3. `~` is expanded using `homeDirectory` via `expandHomePath` (targeting the shell's invocation directory).
  4. For each directory (`~`, `.`, `..`), calls `listDirectory` with `longFormat = true`, displaying file mode (`printPermissions`), link count, owner, group, file size, timestamp, and filename.
  5. For regular file `lines.txt`, `S_ISDIR` is false; calls `printLongEntry("", "lines.txt")` to print its detailed attributes.
* **Result**: **PASS**

---

### Step 10: `ls no_such_file`
* **Input**: `ls no_such_file`
* **Workflow**:
  1. `executeLs` receives path `"no_such_file"`.
  2. Calls `stat("no_such_file", &info)`.
  3. `stat` returns `-1` (`ENOENT`).
  4. `perror("ls")` prints `ls: No such file or directory`.
* **Result**: **PASS**

---

### Step 11: `search search.txt; search count.txt; search Test`
* **Input**: `search search.txt; search count.txt; search Test`
* **Workflow**:
  1. **`search search.txt`**: `searchRecursive(".", "search.txt", found)` walks directories recursively. Recursively descends into `./Test`, matches `search.txt`, sets `found = true`, prints `True`.
  2. **`search count.txt`**: Recursive walk finds no match. Prints `False`.
  3. **`search Test`**: Directory `Test` is located at `./Test`. Matches directory name, sets `found = true`, prints `True`.
* **Result**: **PASS**

---

### Step 12: `echo FIRST > check_output.txt`
* **Input**: `echo FIRST > check_output.txt`
* **Workflow**:
  1. `executeCommand` detects redirection operator `>`.
  2. `saveStandardDescriptors` dups `STDOUT_FILENO`.
  3. `handleRedirection` opens `check_output.txt` with `O_WRONLY | O_CREAT | O_TRUNC, 0644`.
  4. `dup2(fd, STDOUT_FILENO)` points standard output to the file.
  5. `executeEcho` writes `"FIRST\n"` to stdout (the file).
  6. `restoreStandardDescriptors` restores stdout back to terminal.
* **Result**: **PASS**

---

### Step 13: `echo SECOND > check_output.txt`
* **Input**: `echo SECOND > check_output.txt`
* **Workflow**:
  1. `O_TRUNC` flag in `handleRedirection` truncates existing `check_output.txt`.
  2. `executeEcho` writes `"SECOND\n"`.
  3. File now contains exclusively `SECOND`.
* **Result**: **PASS**

---

### Step 14: `echo THIRD >> check_output.txt; cat < check_output.txt`
* **Input**: `echo THIRD >> check_output.txt; cat < check_output.txt`
* **Workflow**:
  1. **`echo THIRD >> check_output.txt`**: `O_APPEND` flag opens file at end-of-file. Writes `"THIRD\n"`.
  2. **`cat < check_output.txt`**: `handleRedirection` opens `check_output.txt` with `O_RDONLY` and redirects stdin via `dup2(fd, STDIN_FILENO)`.
  3. `cat` reads from redirected stdin and outputs `SECOND\nTHIRD`.
* **Result**: **PASS**

---

### Step 15: `sort < lines.txt > lines_sorted.txt; cat < lines_sorted.txt`
* **Input**: `sort < lines.txt > lines_sorted.txt; cat < lines_sorted.txt`
* **Workflow**:
  1. **`sort < lines.txt > lines_sorted.txt`**:
     * Child process is forked for system program `sort`.
     * `handleRedirection` applies `< lines.txt` to stdin and `> lines_sorted.txt` to stdout.
     * `execvp("sort", ["sort"])` sorts lines alphabetically into `lines_sorted.txt`.
  2. **`cat < lines_sorted.txt`**:
     * Reads `lines_sorted.txt` and prints sorted entries (`aa`, `banana`, `carrot`, `cat`).
* **Result**: **PASS**

---

### Step 16: `cat lines.txt | wc`
* **Input**: `cat lines.txt | wc`
* **Workflow**:
  1. `hasPipe` returns true. Calls `executePipeline`.
  2. Creates pipe with `pipefds[0]` (read) and `pipefds[1]` (write).
  3. **Child 0 (`cat lines.txt`)**: `dup2(pipefds[1], STDOUT_FILENO)`. Executes `cat`.
  4. **Child 1 (`wc`)**: `dup2(pipefds[0], STDIN_FILENO)`. Executes `wc`.
  5. Parent closes pipe ends, waits for children. `wc` counts lines, words, bytes from pipe and prints `4 4 27`.
* **Result**: **PASS**

---

### Step 17: `cat lines.txt | head -3 | tail -2`
* **Input**: `cat lines.txt | head -3 | tail -2`
* **Workflow**:
  1. Three-command pipeline: creates 2 pipes (4 file descriptors).
  2. **Child 0 (`cat lines.txt`)**: outputs 4 lines to Pipe 1.
  3. **Child 1 (`head -3`)**: reads from Pipe 1, filters first 3 lines (`aa`, `cat`, `banana`), writes to Pipe 2.
  4. **Child 2 (`tail -2`)**: reads from Pipe 2, filters last 2 lines (`cat`, `banana`), writes to stdout.
* **Result**: **PASS**

---

### Step 18: `ls | grep ".txt" > out.txt; cat < in.txt | wc -l >> out.txt; cat out.txt`
* **Input**: `ls | grep ".txt" > out.txt; cat < in.txt | wc -l >> out.txt; cat out.txt`
* **Workflow**:
  1. **`ls | grep ".txt" > out.txt`**:
     * Pipeline between `ls` and `grep`.
     * `parseArguments` strips quotes from `".txt"` -> `.txt`.
     * Child 0 executes `ls`. Child 1 applies redirection `> out.txt` and executes `grep .txt`. Matching `.txt` files written to `out.txt`.
  2. **`cat < in.txt | wc -l >> out.txt`**:
     * Child 0 redirects stdin from `in.txt`, executes `cat`.
     * Child 1 applies `>> out.txt`, reads from pipe, counts lines (`7`), appends `7` to `out.txt`.
  3. **`cat out.txt`**: Displays `.txt` files followed by `7`.
* **Result**: **PASS**

---

### Step 19: `search search.txt | cat; pinfo > process_info.txt; cat process_info.txt`
* **Input**: `search search.txt | cat; pinfo > process_info.txt; cat process_info.txt`
* **Workflow**:
  1. **`search search.txt | cat`**:
     * Pipeline with builtin command `search`.
     * Child 0 detects builtin `search`, calls `executeSearch`, outputs `True\n` to pipe, and calls `_exit(0)`.
     * Child 1 executes `cat`, printing `True`.
  2. **`pinfo > process_info.txt`**:
     * Builtin `pinfo` redirects stdout to `process_info.txt`, writes process status and memory, restores descriptors.
  3. **`cat process_info.txt`**: Prints recorded process info.
* **Result**: **PASS**

---

### Step 20: `cat < no_such_input; echo still_alive`
* **Input**: `cat < no_such_input; echo still_alive`
* **Workflow**:
  1. **`cat < no_such_input`**:
     * Child forked for `cat`.
     * `handleRedirection` attempts `open("no_such_input", O_RDONLY)`.
     * `open` returns `-1`. `perror("open")` prints `open: No such file or directory`.
     * Returns `false`. Child immediately calls `_exit(1)` without attempting `execvp`.
  2. Parent reaps child and continues to next command.
  3. **`echo still_alive`**: Executes and prints `still_alive`.
* **Result**: **PASS**

---

### Step 21: `pinfo abc; unknown_aos_command; echo still_alive`
* **Input**: `pinfo abc; unknown_aos_command; echo still_alive`
* **Workflow**:
  1. **`pinfo abc`**: `atoi("abc")` yields `0`. Attempts to open `/proc/0/stat`. Fails, outputs error.
  2. **`unknown_aos_command`**: Child process calls `execvp("unknown_aos_command")`. Fails with `execvp: No such file or directory`, exits with 1.
  3. Parent stays intact and executes `echo still_alive`.
* **Result**: **PASS**

---

### Step 22: `sleep 300 & sleep 300 &`
* **Input**: `sleep 300 & sleep 300 &`
* **Workflow**:
  1. Command 1: `background = true`. Child forked, calls `setpgid(0, 0)`, `execvp("sleep", ["sleep", "300"])`.
  2. Parent records PID1 in `backgroundProcesses` and prints `[PID1] sleep`.
  3. Command 2: Child forked for second sleep. Parent records PID2 and prints `[PID2] sleep`.
  4. Shell prompt immediately returns.
* **Result**: **PASS**

---

### Step 23: `pinfo PID1; pinfo PID2`
* **Input**: `pinfo PID1; pinfo PID2`
* **Workflow**:
  1. `executePinfo` opens `/proc/<PID1>/stat`.
  2. Reads process state character (`S` for interruptible sleep).
  3. Checks foreground: since PID != getpid(), does **not** append `+`. Outputs `process Status -- S`.
  4. Reads memory from `/proc/<PID1>/statm` and executable path from `/proc/<PID1>/exe`.
  5. Repeats for PID2.
* **Result**: **PASS**

---

### Step 24: `sleep 2; echo finished_waiting`
* **Input**: `sleep 2; echo finished_waiting`
* **Workflow**:
  1. `sleep 2` is spawned in foreground (`background = false`).
  2. `foregroundPgid = pid`. Parent blocks inside `waitpid(pid, &status, WUNTRACED)`.
  3. Process sleeps for 2 seconds and exits with 0.
  4. `waitpid` unblocks; shell moves to next command and prints `finished_waiting`.
* **Result**: **PASS**

---

### Step 25 & 26: `sleep 300 | cat`, Ctrl-C
* **Input**: `sleep 300 | cat`, then `Ctrl-C`, then `Ctrl-C` and `Ctrl-Z` at prompt
* **Workflow**:
  1. Pipeline `sleep 300 | cat` launched in foreground. `foregroundPgid` set to pipeline PGID.
  2. User presses `Ctrl-C`.
  3. `handleSigInt` intercepts `SIGINT`. Sends `kill(-foregroundPgid, SIGINT)` to the entire pipeline process group.
  4. `sleep` and `cat` are terminated.
  5. `waitpid` reaps the children, `foregroundPgid` resets to `-1`, and prompt returns.
  6. Subsequent `Ctrl-C` and `Ctrl-Z` at prompt find `foregroundPgid <= 0` and do nothing.
* **Result**: **PASS**

---

### Step 27 & 28: `sleep 300`, Ctrl-Z, `pinfo STOPPED_PID`
* **Input**: `sleep 300`, then `Ctrl-Z`, then `pinfo STOPPED_PID`
* **Workflow**:
  1. `sleep 300` launched in foreground (`foregroundPgid = pid`).
  2. User presses `Ctrl-Z`.
  3. `handleSigTstp` sends `kill(-foregroundPgid, SIGTSTP)`.
  4. Child transitions to stopped state.
  5. Parent's `waitpid` returns with `WIFSTOPPED(status) == true`.
  6. Prints `[<STOPPED_PID>] stopped`.
  7. `pinfo STOPPED_PID` reads `/proc/<STOPPED_PID>/stat`. State character is `'T'`. Prints `process Status -- T`.
* **Result**: **PASS**

---

### Step 29: `ec<TAB>TAB_OK<ENTER>`
* **Input**: Types `ec`, presses `<TAB>`, types `TAB_OK`, presses Enter
* **Workflow**:
  1. Buffer contains `"ec"`. User presses `\t`.
  2. `autocomplete()` activates in `commandMode`.
  3. Checks builtins: finds unique match `"echo"`.
  4. Completes buffer to `"echo "` (with trailing space) and redraws line.
  5. User types `TAB_OK`: buffer becomes `"echo TAB_OK"`.
  6. User presses Enter: `executeEcho` prints `TAB_OK`.
* **Result**: **PASS**

---

### Step 30: `ca<TAB>`, Backspaces
* **Input**: Types `ca`, presses `<TAB>`, then backspaces
* **Workflow**:
  1. Buffer contains `"ca"`. User presses `\t`.
  2. No builtin starts with `ca`. Scans directories in `$PATH`.
  3. Finds multiple matching executables (`cat`, `capsh`, `caller`, etc.).
  4. Calculates common prefix `"ca"`.
  5. Prints all matching executables and redraws prompt with `ca`.
  6. User sends backspace (`\x7f`): removes `'a'`, then `'c'`. Prompt is clean.
* **Result**: **PASS**

---

### Step 31: `cat lin<TAB><TAB>_<TAB><ENTER>`
* **Input**: Types `cat lin`, presses `<TAB>` twice, types `_`, presses `<TAB>`, presses Enter
* **Workflow**:
  1. Word prefix is `"lin"`. `commandMode = false` (preceded by `cat `).
  2. Scans current directory with `opendir(".")`. Matches `lines.txt` and `lines_sorted.txt`.
  3. Computes common prefix `"lines"`. Buffer expands to `cat lines`.
  4. Shows matches `lines.txt  lines_sorted.txt` and redraws line.
  5. User types `_`: buffer becomes `cat lines_`.
  6. User presses `<TAB>`: single match `lines_sorted.txt`. Buffer expands to `cat lines_sorted.txt`.
  7. Enter executes `cat lines_sorted.txt`, displaying sorted lines.
* **Result**: **PASS**

---

### Step 32: `echo arrow_test`, `<UP>`, `_edited`, Enter
* **Input**: `echo arrow_test`, `<UP>`, `_edited`, Enter
* **Workflow**:
  1. Runs `echo arrow_test`. Appended to history.
  2. Next prompt: user sends escape sequence `\033[A` (UP arrow).
  3. `historyIndex` moves to previous command (`echo arrow_test`).
  4. Line buffer populated with `echo arrow_test` and displayed.
  5. User types `_edited`: buffer becomes `echo arrow_test_edited`.
  6. Pressing Enter executes `echo arrow_test_edited`. Outputs `arrow_test_edited`.
* **Result**: **PASS**

---

### Step 33: `<UP>`, `<UP>`, `<DOWN>`, Enter
* **Input**: `<UP>`, `<UP>`, `<DOWN>`, Enter
* **Workflow**:
  1. `<UP>`: loads `echo arrow_test_edited`.
  2. `<UP>`: loads older command `echo arrow_test`.
  3. `<DOWN>` (`\033[B`): moves forward in history back to `echo arrow_test_edited`.
  4. Enter executes `echo arrow_test_edited`.
* **Result**: **PASS**

---

### Step 34: `history`
* **Input**: `history`
* **Workflow**:
  1. `executeHistory` called with `argc = 1`.
  2. Default count is 10. `start = max(0, historyCount - 10)`.
  3. Prints the last 10 commands recorded in `historyCommands`.
* **Result**: **PASS**

---

### Step 35: `history 19`
* **Input**: `history 19`
* **Workflow**:
  1. `executeHistory` parses `args[1] = "19"`.
  2. Verifies `num > 0 && num < 20`.
  3. Computes `start = max(0, historyCount - 19)`.
  4. Prints the last 19 commands recorded in the session.
* **Result**: **PASS**

---

### Step 36: `exit`
* **Input**: `exit`
* **Workflow**:
  1. `readCommandWithHistory` reads `exit`.
  2. `addToHistory("exit")` persists `exit` to `.shell_history`.
  3. `main.cpp` checks `if (strcmp(command, "exit") == 0) break;`.
  4. Breaks out of the main loop and exits with status 0.
* **Result**: **PASS**

---

### Step 37: Shell Restart, `history 5`
* **Input**: Start shell again, enter `history 5`
* **Workflow**:
  1. New shell process starts.
  2. `initializeHistory` opens `<homeDirectory>/.shell_history`.
  3. Reads history commands persisted from previous session.
  4. User enters `history 5`.
  5. Prints the last 5 commands from across sessions (including `exit` from previous session).
* **Result**: **PASS**

---

### Step 38: `Ctrl-D`
* **Input**: `Ctrl-D` (`\x04`)
* **Workflow**:
  1. `readCommandWithHistory` reads character code `4` on an empty line.
  2. Restores original terminal `termios` settings.
  3. Prints `\n`, flushes stdout, and returns `-1`.
  4. `main.cpp` detects negative return code and breaks main loop.
  5. Shell process terminates cleanly.
* **Result**: **PASS**
