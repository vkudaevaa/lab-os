#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

bool validate_string(const char *str) {
    size_t len = strlen(str);
    if (len == 0) return false;
    return (str[len - 1] == '.' || str[len - 1] == ';');
}

int main(int argc, char *argv[]) {
    if (argc < 2){
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];
    char error_message[] = "Invalid string\n";

    // Открытие файла для записи строк
    int fd = open(argv[1], O_WRONLY | O_TRUNC | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_read;
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Child: received string '%s', %zd bytes read\n", buffer, bytes_read);
        // Проверка строки на валидность
        if (validate_string(buffer)) {
            write(fd, buffer, strlen(buffer));
            write(fd, "\n", 1);
            {
                const char msg[] = "OK\n";
                write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            }
        } else {
            write(STDOUT_FILENO, error_message, sizeof(error_message) - 1);
        }
    }
    if (bytes_read == -1) {
        perror("Failed to read from stdin");
    }
    close(fd);
    exit(EXIT_SUCCESS);
}
