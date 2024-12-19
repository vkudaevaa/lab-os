#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define BUFFER_SIZE 1024
#define SHM_SIZE sizeof(char) * 1024 

int main() {
    int shm_fd;
    char *shm_ptr;
    sem_t *sem_write, *sem_read;
    pid_t pid;
    char buffer[BUFFER_SIZE];
    char filename[256];

    shm_fd = shm_open("/shm_example", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    sem_write = sem_open("/sem_write", O_CREAT, S_IRUSR | S_IWUSR, 0); 
    sem_read = sem_open("/sem_read", O_CREAT, S_IRUSR | S_IWUSR, 1);  

    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    write(STDOUT_FILENO, "Enter the file name to record: ", 32);
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
        close(shm_fd);

        while (1) {
            sem_wait(sem_read);

            if (strcmp(shm_ptr, "exit") == 0) {
                break;  
            }

            execl("./child", "child", filename, NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {  
        close(shm_fd);

        write(STDOUT_FILENO, "Enter a line (or 'exit' to exit): ", 35);
        while ((len = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
            if (buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            if (strcmp(buffer, "exit") == 0) {
               
                memcpy(shm_ptr, "exit", 5);
                sem_post(sem_read); 
                break;
            }

            memcpy(shm_ptr, buffer, strlen(buffer) + 1);

            sem_post(sem_read);  
            sem_wait(sem_write); 

            write(STDOUT_FILENO, "Enter a line (or 'exit' to exit): ", 35);
        }
        
        sem_post(sem_read);  
        sem_close(sem_write);
        sem_close(sem_read);
        sem_unlink("/sem_write");
        sem_unlink("/sem_read");

        munmap(shm_ptr, SHM_SIZE);
        shm_unlink("/shm_example");

        wait(NULL);
        exit(EXIT_SUCCESS);
    }

    return 0;
}
