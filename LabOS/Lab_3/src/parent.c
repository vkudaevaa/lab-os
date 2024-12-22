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
#define SHM_NAME "/shared_memory"
#define SEM_WRITE_NAME "/sem_write" 
#define SEM_READ_NAME "/sem_read"
#define SHM_SIZE sizeof(char) * 1024 

int main() {
    int shm_fd;
    char *shm_ptr;
    sem_t *sem_write, *sem_read;
    pid_t pid;
    char buffer[BUFFER_SIZE];
    char filename[256];

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("Error creating shared memory\n");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("Error resizing shared memory\n");
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Error mapping shared memory\n");
        exit(EXIT_FAILURE);
    }

    sem_write = sem_open(SEM_WRITE_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0); 
    sem_read = sem_open(SEM_READ_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);  

    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED) {
        perror("Error creating semaphore\n");
        exit(EXIT_FAILURE);
    }

    write(STDOUT_FILENO, "Enter the file name to record: ", 32);
    ssize_t len = read(STDIN_FILENO, filename, sizeof(filename));
    if (filename[len - 1] == '\n') {
        filename[len - 1] = '\0';
    }

    pid = fork();

    if (pid < 0) {
        perror("Fork failed\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        close(shm_fd);

        sem_wait(sem_read);
        execl("./child", "child", filename, NULL);
        perror("Execl failed\n");
        exit(EXIT_FAILURE);
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
        }
        
        sem_post(sem_read);  
        sem_close(sem_write);
        sem_close(sem_read);
        sem_unlink(SEM_WRITE_NAME);
        sem_unlink(SEM_READ_NAME);

        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);

        wait(NULL);
        exit(EXIT_SUCCESS);
    }

    return 0;
}