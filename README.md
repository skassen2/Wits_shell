# witsshell

`witsshell` is a simple Unix-like shell implemented in C. It supports basic command execution, built-in commands (`exit`, `cd`, and `path`), output redirection, and parallel command execution.

## 📄 Description

This shell executes commands typed in by the user (interactive mode) or from a file (batch mode).  
It supports:
- Command parsing and execution
- Built-in commands: `exit`, `cd`, `path`
- Output redirection with `>`
- Parallel command execution using `&`

## 🔧 Compilation

To compile the program:

```bash
gcc -o witsshell witsshell.c


