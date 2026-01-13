# Shell Project


## Overview

Welcome to My Custom Shell project! This project is a custom shell program implemented in C++ that simulates basic shell functionalities, including running commands, handling pipes, input/output redirection, and background job execution. It mimics some core aspects of traditional shells like bash or zsh.

## Features

- 🖥️ **Current Directory Prompt**: Displays the current working directory as part of the shell prompt.
- 🚀 **Command Execution**: Ability to execute external commands and built-in commands like `cd`.
- 🔀 **Input/Output Redirection**: Supports input (`<`) and output (`>`) redirection.
- 🔗 **Pipelining**: Supports command pipelines using the `|` operator.
- 🏃‍♂️ **Background Jobs**: Run processes in the background using `&` and manage them with a built-in `myjobs` command.

## How It Works

This shell allows users to type commands, which are parsed and then executed. It handles special cases like:

- Changing directories (`cd`)
- Listing active jobs (`myjobs`)
- Redirection of input/output to files
- Command pipelines, where the output of one command becomes the input for the next
- Running processes in the background and keeping track of these jobs

## Implementation Details

### 1. Command Parsing
- Commands are first split using the pipe (`|`) symbol to handle piping
- Each command is further split into its arguments
- Whitespace trimming is applied to ensure clean inputs

### 2. Handling Redirections
- The shell checks for `>` and `<` symbols to manage output and input redirection to/from files
- Redirection is managed using system calls like `dup2()`

### 3. Pipelining
- If commands are piped, the shell sets up a series of pipes between commands to transfer outputs between them

### 4. Job Control
- Background jobs are launched by appending `&` to the command
- The shell keeps track of job statuses and can display active and inactive jobs using the `myjobs` command

## Skills and Technologies Used

<table>
  <tr>
    <th>Skill</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>🔧 C++ Programming</td>
    <td>Mastery of C++ was critical for implementing the shell's core functionality, including process management, file handling, and string manipulation.</td>
  </tr>
  <tr>
    <td>🔧 UNIX System Programming</td>
    <td>
      - System calls like `fork()`, `execv()`, `waitpid()`, and `pipe()` were essential for creating new processes and handling input/output redirection.<br>
      - File descriptors were used to manage input/output redirection and pipes between processes.
    </td>
  </tr>
  <tr>
    <td>🔧 File System Management</td>
    <td>Used `<filesystem>` for managing directory paths and handling file input/output.</td>
  </tr>
  <tr>
    <td>🔧 Inter-Process Communication (IPC)</td>
    <td>Implemented process piping using `pipe()` and managed communication between processes.</td>
  </tr>
  <tr>
    <td>🔧 Memory Management</td>
    <td>Dynamic memory management was handled carefully, especially in constructing arguments for `execv()`.</td>
  </tr>
</table>


## Getting Started

```bash
# Clone the repository
git clone https://github.com/yourusername/custom-shell.git

# Change to the project directory
cd custom-shell

# Compile the project
make

# Run the shell
./custom-shell
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
