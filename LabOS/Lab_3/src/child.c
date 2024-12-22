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
#define SHM_NAME "/shared_memory"
#define SEM_WRITE_NAME "/sem_write" 
#define SEM_READ_NAME "/sem_read"
#define SHM_SIZE sizeof(char) * 1024  

bool validate_string(const char *str) {
    size_t len = strlen(str);
    if (len == 0) return false;
    return (str[len - 1] == '.' || str[len - 1] == ';');
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int shm_fd;
    char *shm_ptr;
    sem_t *sem_write, *sem_read;
    char buffer[BUFFER_SIZE];
    char filename[256];
    
    int fd = open(argv[1], O_WRONLY | O_TRUNC | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            perror("Error open input file\n");
            exit(EXIT_FAILURE);
        }

    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0);
    if (shm_fd == -1) {
        perror("Error creating shared memory\n");
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Error mapping shared memory\n");
        exit(EXIT_FAILURE);
    }

    sem_write = sem_open(SEM_WRITE_NAME, 0);
    sem_read = sem_open(SEM_READ_NAME, 0);
    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED) {
        perror("Error creating semaphore\n");
        exit(EXIT_FAILURE);
    }

    char error_message[] = "Invalid string\n";

    while (1) {
        sem_wait(sem_read);  

        if (strcmp(shm_ptr, "exit") == 0) {
            break;
        }

        if (validate_string(shm_ptr)) {
          
            write(fd, shm_ptr, strlen(shm_ptr));
            write(fd, "\n", 1);

        } else {
            write(STDOUT_FILENO, error_message, sizeof(error_message) - 1);
        }

        sem_post(sem_write);
    }

    close(fd);
    sem_close(sem_write);
    sem_close(sem_read);
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);
    exit(EXIT_SUCCESS);
}