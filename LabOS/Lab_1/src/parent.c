#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

#define BUFFER_SIZE 1024

int main() {
    int pipe1[2], pipe2[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    char resp_buffer[BUFFER_SIZE];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    write(STDOUT_FILENO, "Enter the file name to record: ", 32);
    char filename[256];
    ssize_t len = read(STDIN_FILENO, filename, sizeof(filename));
    if (filename[len - 1] == '\n') {
        filename[len - 1] = '\0';
    }

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        dup2(pipe1[STDIN_FILENO], STDIN_FILENO);
        dup2(pipe2[STDOUT_FILENO], STDOUT_FILENO);
        close(pipe1[STDOUT_FILENO]);
        close(pipe2[STDIN_FILENO]);

        execl("./child", "child", filename, NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        close(pipe1[STDIN_FILENO]);
        close(pipe2[STDOUT_FILENO]);

        write(STDOUT_FILENO, "Enter a line (or 'exit' to exit): ", 35);
        while ((len = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
            
            if (buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            if (strcmp(buffer, "exit") == 0) {
                break;
            }

            write(pipe1[STDOUT_FILENO], buffer, strlen(buffer) + 1);

            ssize_t bytes_read = read(pipe2[STDIN_FILENO], resp_buffer, sizeof(resp_buffer) - 1);
            if (bytes_read > 0) {
                resp_buffer[bytes_read] = '\0';
                if (strcmp(resp_buffer, "OK\n") != 0) {
                    write(STDOUT_FILENO, "Error: ", 8);
                    write(STDOUT_FILENO, resp_buffer, bytes_read);
                }
            }
        }

        close(pipe1[STDOUT_FILENO]);
        close(pipe2[STDIN_FILENO]);
        wait(NULL);
        exit(EXIT_SUCCESS);
    }

    return 0;
}
