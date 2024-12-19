#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define SHM_SIZE sizeof(char) * 1024 

bool validate_string(const char *str) {
    size_t len = strlen(str);
    if (len == 0) return false;
    return (str[len - 1] == '.' || str[len - 1] == ';');
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("File name not provided");
        exit(EXIT_FAILURE);
    }

    int shm_fd;
    char *shm_ptr;
    sem_t *sem_write, *sem_read;
    char buffer[BUFFER_SIZE];
    char filename[256];
    
    strncpy(filename, argv[1], sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    shm_fd = shm_open("/shm_example", O_RDONLY, 0);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    sem_write = sem_open("/sem_write", 0);
    sem_read = sem_open("/sem_read", 0);
    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem_read);  

        if (strcmp(shm_ptr, "exit") == 0) {
            break;
        }

        if (validate_string(shm_ptr)) {
          
            int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
            if (fd == -1) {
                perror("Failed to open file");
                exit(EXIT_FAILURE);
            }

            write(fd, shm_ptr, strlen(shm_ptr));
            write(fd, "\n", 1);
            close(fd);
            write(STDOUT_FILENO, "OK\n", 3);
        } else {
            write(STDOUT_FILENO, "Invalid string\n", 15);
        }

        sem_post(sem_write);
    }

    sem_close(sem_write);
    sem_close(sem_read);
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink("/shm_example");

    exit(EXIT_SUCCESS);
}
