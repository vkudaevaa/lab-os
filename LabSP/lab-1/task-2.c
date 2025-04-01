#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4096
#define BUF_SIZE 1024
#define MAX_LINE 1024
#define PADDING 0x00

typedef enum errorCode{
	OPT_SUCCESS,
    OPT_ERROR_OPTION,
    OPT_ERROR_INVALID_FORMAT,
    OPT_ERROR_INVALID_ARGUMENTS,
    OPT_ERROR_OPEN_FILE,
	OPT_ERROR_MEMORY
} errorCode;


errorCode xorN(const char* filename, int N, uint8_t* xor_result) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return OPT_ERROR_OPEN_FILE;
    }
    
    int block_size = (N > 2) ? 1 << (N - 3) : 1;
    memset(xor_result, 0, block_size);
    uint8_t* buffer = (uint8_t*)calloc(block_size, sizeof(uint8_t));
    if (!buffer) {
        fclose(file);
        return OPT_ERROR_MEMORY;
    }

    
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, block_size, file)) > 0) {
        if (bytes_read < (size_t)block_size) {
			memset(buffer + bytes_read, PADDING, block_size - bytes_read);
		}
        if (N > 2){
            for (size_t i = 0; i < bytes_read; i++) {
                xor_result[i] ^= buffer[i];
            }

        }else{
            uint8_t high = (buffer[0] >> 4) & 0x0F; 
            uint8_t low  = buffer[0] & 0x0F;       

            xor_result[0] ^= high;  
            xor_result[0] ^= low;   

        }
        
    }
    
    free(buffer);
    fclose(file);
    return OPT_SUCCESS;
}

errorCode mask_count(const char* filename, uint32_t mask, int* count) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return OPT_ERROR_OPEN_FILE;
    }

    uint32_t value;
    while (fread(&value, sizeof(uint32_t), 1, file) == 1) {
        if ((value & mask) == mask) (*count)++;
    }
    fclose(file);
    return OPT_SUCCESS;
}

void copy_file(const char *src, const char *dst) {
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("Error open input file!");
        exit(EXIT_FAILURE);
    }

    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("Error making a copy of the file!");
        close(src_fd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error writing to a file!");
            close(src_fd);
            close(dst_fd);
            exit(EXIT_FAILURE);
        }
    }

    close(src_fd);
    close(dst_fd);
}

void copyN(const char *filename, int num_copies) {
    for (int i = 1; i <= num_copies; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork failed!");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { 
            char new_filename[256];
            snprintf(new_filename, sizeof(new_filename), "%s-%d", filename, i);
            copy_file(filename, new_filename);
            exit(EXIT_SUCCESS);
        }
    }

    for (int i = 0; i < num_copies; i++) {
        wait(NULL);
    }
}


int find_string(const char* filename, const char* pattern) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed!");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        FILE *file = fopen(filename, "r");
        if (!file) {
            perror("Error open input file!");
            exit(EXIT_FAILURE);
        }

		size_t pattern_len = strlen(pattern);
		char buffer[BUF_SIZE + 1]; 
		size_t read_size;
	
		while ((read_size = fread(buffer, 1, BUF_SIZE, file)) > 0) {
			buffer[read_size] = '\0'; 
	
			if (strstr(buffer, pattern)) {
				fclose(file);
                printf("find(%s, %s) = 1\n", filename, pattern);
                exit(EXIT_SUCCESS);
			}
	
			if (read_size > pattern_len) {
				fseek(file, -(long)pattern_len, SEEK_CUR);
			}
		}

        fclose(file);
        printf("find(%s, %s) = 0\n", filename, pattern);
        exit(EXIT_SUCCESS);
    }else{
        wait(NULL);
    }
    
}

int optionValidation(const char *opt) {
    char *endptr;
    long res = strtol(opt, &endptr, 10);
    if (res < 0 || res > INT_MAX) {
        return -1;
    }
    if (endptr == opt || *endptr != '\0') {
        return -1;
    }
    return (int)res; 
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Error: Invalid numbers of arguments!\n");
        fprintf(stderr, "Usage: %s <file1> <file2> ... <option>\n", argv[0]);
        return OPT_ERROR_INVALID_ARGUMENTS;
    }

    char* operation = argv[argc - 2]; 
    errorCode opt;

    for (int i = 1; i < argc - 2; i++) {
        char* filename = argv[i];

        if (strcmp(operation, "xor") == 0) {
            int N = optionValidation(argv[argc - 1]);
            if (N < 2 || N > 6){
                fprintf(stderr, "Error: Invalid operation format!\n");
                return OPT_ERROR_INVALID_FORMAT;
            }
            int block_size = (N > 2) ? 1 << (N - 3) : 1;
            uint8_t* xor_result = (uint8_t*)malloc(sizeof(uint8_t) * block_size);
            opt = xorN(filename, N, xor_result);
            if (opt == OPT_ERROR_MEMORY){
                fprintf(stderr, "Error: Memory allocation error!\n");
                return OPT_ERROR_MEMORY;
            }else if (opt == OPT_ERROR_OPEN_FILE){
                fprintf(stderr, "Error: Error open input file\n");
                return OPT_ERROR_OPEN_FILE;
            }else{
                printf("xor%d(%s) = ", N, filename);
                for (int j = 0; j < block_size; ++j) {
					printf("%02x", xor_result[j]);
				}
				printf("\n");
            }
            free(xor_result);

        } else if (strcmp(operation, "mask") == 0) {
            char* endptr;
            uint32_t mask = strtoul(argv[argc - 1], &endptr, 16);
            if (*endptr != '\0') {
                fprintf(stderr, "Error: Invalid operation format!\n");
                return OPT_ERROR_INVALID_FORMAT;
            }
            int count = 0;
            opt = mask_count(filename, mask, &count);
            if (opt == OPT_ERROR_OPEN_FILE){
                fprintf(stderr, "Error: Error open input file\n");
                return OPT_ERROR_OPEN_FILE;
            }
            printf("mask(%s, 0x%X) = %d\n", filename, mask, count);

        } else if (strcmp(operation, "copy") == 0) {
            int N = optionValidation(argv[argc - 1]);
            if (N < 1 || N > 100){
                return OPT_ERROR_INVALID_FORMAT;
            }
            copyN(filename, N);
        } else if (strcmp(operation, "find") == 0) {
            char* some_string = argv[argc - 1];
            find_string(filename, some_string); 
        } else {
            fprintf(stderr,"Error: Unknown operation!\n");
            return OPT_ERROR_OPTION;
        }
    }

    return OPT_SUCCESS;
}
