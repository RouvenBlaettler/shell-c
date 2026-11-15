# shell-c

A POSIX-style Unix shell written from scratch in C. It parses command lines, runs built-in commands and external programs, and supports pipelines, I/O redirection and shell quoting rules.

```console
$ echo "hello   world" | tr a-z A-Z
HELLO   WORLD
$ ls / | grep bin | wc -l
4
$ echo hi > out.txt
$ cat < out.txt
hi
$ ls /nonexistent 2> errors.txt
$ type cd
cd is a shell builtin
```

## Features

- **External programs**: resolves commands via `$PATH` and runs them with `fork` + `execv`
- **Pipelines**: any number of stages (`a | b | c`), wired together with `pipe` and `dup2`
- **I/O redirection**: `>` / `1>`, `>>` / `1>>`, `2>`, `2>>`, `<`, also inside individual pipeline segments
- **Quoting**: single quotes, double quotes, and backslash escaping (inside and outside double quotes)
- **Built-ins**: `cd` (incl. `~`), `pwd`, `echo`, `type`, `exit`

## How it works

1. **Read**: a REPL reads one line of input.
2. **Tokenize**: a hand-written tokenizer splits the line into arguments, applying quoting and escape rules and treating `|` as its own token.
3. **Redirect**: redirection operators are parsed out of the token list; the target file is opened and the file descriptor is redirected with `dup2` (and restored afterwards for built-ins).
4. **Execute**:
   - built-ins run directly in the shell process (they have to, e.g. `cd` changes the shell's own working directory);
   - external commands are found by searching each directory in `$PATH` for an executable file, then run in a child process;
   - for pipelines, one pipe is created between each pair of stages and one child is forked per stage, with its stdin/stdout connected to the neighbouring pipes.

## Build & run

Requires a C compiler on Linux or macOS.

```sh
gcc -std=gnu2x src/main.c -o shell
./shell
```

Or with CMake (also links `readline`):

```sh
cmake -B build -S .
cmake --build build
./build/shell
```

## Background

Built while working through the [CodeCrafters "Build Your Own Shell"](https://app.codecrafters.io/courses/shell/overview) challenge, to understand processes, file descriptors and parsing at the operating-system level.
