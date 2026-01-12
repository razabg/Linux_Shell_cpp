#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <fcntl.h> // For open()

/*
 * Function to print the current directory location.
 */
void print_dir_location() {
    auto currentPath = std::filesystem::current_path();
    std::cout << currentPath.string() << " > ";
}


/*
* Function that trim any white spaces in the head or tail of the command input
*/
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) {
        return ""; // The string is all whitespace
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);//return the relevant content form the str without spaces
}

/*
 * Function to split a string into a vector of strings using a given delimiter.
 * param str: The input string to be split.
 * param delimiter: The character used to separate the string.
 * return: A vector of strings resulting from the split.
 */
std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) { //parse the string like an input
        tokens.push_back(token);
    }
    return tokens;
}

/*
 * Function to find the executable file for a given command in the PATH directories.
 * param exe_name: The name of the executable to find.
 * return: The full path of the executable if found, otherwise an empty string.
 */
std::string find_executable(const std::string &exe_name) {
    std::string path = std::getenv("PATH"); // get the environment variables
    std::vector<std::string> directories = split(path, ':'); //split and arrange the directories into vector of strings

    for (const auto &dir : directories) {
        std::filesystem::path exe_path = std::filesystem::path(dir) / exe_name;
        if (std::filesystem::exists(exe_path) && std::filesystem::is_regular_file(exe_path)) {
            return exe_path.string();
        }
    }
    return "";
}

/*
 * Function to handle input redirection.
 * param input_file: The file to be used as input.
 * return: The file descriptor of the opened file or -1 on failure.
 */
int handle_input_redirection(const std::string& input_file) {
    int fd = open(input_file.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("open for input");
    }
    return fd;
}

/*
 * Function to handle output redirection.
 * param output_file: The file to be used as output.
 * return: The file descriptor of the opened file or -1 on failure.
 */
int handle_output_redirection(const std::string& output_file) {
    int fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open for output");
    }
    return fd;
}


/*
 * Function to prepare command arguments for execv.
 * param command: The command vector containing the executable and its arguments.
 * return: A dynamically allocated array of C strings.
 */
const char** prepare_execv_args(const std::vector<std::string>& command) {
    size_t argc = command.size();
    const char **argv = new const char* [argc + 1];
    for (size_t j = 0; j < argc; ++j) {
        argv[j] = command[j].c_str();
    }
    argv[argc] = nullptr;
    return argv;
}

/*
 * Function to execute a single command with arguments, handling both foreground and background processes.
 * param command: A vector of strings representing the command and its arguments.
 * param path: The path to the executable file.
 * param Jobs: A map to track running jobs, with PID as the key and job status as the value.
 * param input_fd: The file descriptor to be used as input (for pipes).
 * param output_fd: The file descriptor to be used as output (for pipes).
 */
void exe_single_command(std::vector<std::string>& command, const std::string& path,
                        std::unordered_map<pid_t, std::string>& Jobs, int input_fd, int output_fd) {
    bool background = false;
    std::string input_file, output_file;

    // Handle background process
    if (command.back() == "&") {
        background = true;
        command.pop_back();
    }

    // Check for input/output redirection
    for (size_t i = 0; i < command.size(); ++i) {
        if (command[i] == "<" && i + 1 < command.size()) { //ensure that after "<" there is another element
            input_file = command[i + 1];
            command.erase(command.begin() + i, command.begin() + i + 2); //removes both the < symbol and the file name from the command vector.
            --i; //decrease i  because command size got smaller
        } else if (command[i] == ">" && i + 1 < command.size()) {
            output_file = command[i + 1];
            command.erase(command.begin() + i, command.begin() + i + 2);
            --i;
        }
    }

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork failed" << std::endl;
        return;
    } else if (pid == 0) {
        // Child process
        if (!input_file.empty()) {
            input_fd = handle_input_redirection(input_file);
        }
        if (!output_file.empty()) {
            output_fd = handle_output_redirection(output_file);
        }

        // Redirect input/output if needed (for pipes or actual redirection like "<" or ">")
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Prepare arguments and execute
        const char **argv = prepare_execv_args(command);
        execv(path.c_str(), (char **)argv); //run the command

        std::cerr << "execv failed" << std::endl;
        delete[] argv;
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        Jobs[pid] = command[0] + ": this process is no longer active";

        if (!background) {
            waitpid(pid, nullptr, 0);
        } else {
            std::cout << "Process running in background with PID: " << pid << std::endl;
        }

        // Update job status
        if (kill(pid, 0) == 0) {
            Jobs[pid] = command[0] + ": this process is active";
        }
    }
}
/*
 * Function to execute a command pipeline.
 * param commands: A vector of commands (each command is a vector of strings).
 * param Jobs: A map to track running jobs, with PID as the key and job status as the value.
 */
void execute_pipeline(std::vector<std::vector<std::string>>& commands, std::unordered_map<pid_t,std::string>& Jobs) {
    int num_commands = commands.size();
    int pipe_fd[2];
    int input_fd = -1;

    for (int i = 0; i < num_commands; ++i) { // loop that run every command in the pipeline
        if (i != num_commands - 1) {
            if (pipe(pipe_fd) == -1) { //creating the pipe
                std::cerr << "Pipe failed" << std::endl;
                return;
            }
        }

        std::string exe_path = find_executable(commands[i][0]);
        if (exe_path.empty()) {
            std::cerr << commands[i][0] << ": command not found" << std::endl;
            return;
        }

        int output_fd = (i == num_commands - 1) ? -1 : pipe_fd[1]; // if its tha last command of the pipeline,
        // output_fd will be equal to -1 so we will not change the output when the command will run

        exe_single_command(commands[i], exe_path, Jobs, input_fd, output_fd);

        if (input_fd != -1) {
            close(input_fd);
        }
        if (output_fd != -1) {
            close(output_fd);
        }
        input_fd = pipe_fd[0]; //reset the input fd to be pipe read again
    }
}

/*
 * Function to change the current working directory.
 * param path: The path to the new working directory.
 */
void cd(const std::string& path) {
    if (chdir(path.c_str()) != 0) {
        perror("chdir failed");
    }
}

/*
 * Function to print the list of jobs with their status.
 * param Jobs: A map of job PIDs to job status strings.
 */
void myjob(std::unordered_map<pid_t,std::string>& Jobs) {
    for (const auto& job : Jobs) {
        std::cout << job.first << ":" << " \t " << job.second << std::endl;
    }
}

int main() {
    std::string input;
    std::unordered_map<pid_t,std::string> Jobs;

    while (true) {
        print_dir_location();
        std::getline(std::cin, input);

        if (input == "exit") { break; } // Exit the shell

        std::vector<std::string> command_parts = split(input, '|'); // Split by pipes
        std::vector<std::vector<std::string>> commands;

        for (auto& part : command_parts) {
            part = trim(part);  // Trim leading and trailing whitespace
            std::vector<std::string> command = split(part, ' ');
            if (!command.empty()) {
                commands.push_back(command);
            }
        }

        if (commands.empty()) { continue; } // If no command was entered, continue

        if (commands[0][0] == "cd") {
            if (commands[0].size() > 1) {
                cd(commands[0][1]);
            } else {
                std::cerr << "cd: missing argument" << std::endl;
            }
            continue;
        }
        if (commands[0][0] == "myjobs") {
            myjob(Jobs);
            continue;
        }

        // Execute the pipeline
        execute_pipeline(commands, Jobs);
    }

    return 0;
}
