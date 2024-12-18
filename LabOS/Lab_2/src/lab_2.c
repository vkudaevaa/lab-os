#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define ROWS 5
#define COLS 5

typedef struct {
    int start_row;
    int end_row;
    double **matrix_erosion;
    double **result_erosion;
    double **matrix_dilation;
    double **result_dilation;
} Task;

pthread_mutex_t thread_count_mutex;
pthread_cond_t thread_available;
int active_threads = 0;

void write_message(const char* message) {
    if (write(STDOUT_FILENO, message, strlen(message)) == -1) {
        perror("Error: Error writing message\n");
        exit(EXIT_FAILURE);
    }
}

double** allocateMatrix(int rows, int cols) {
    double** matrix = (double**)malloc(rows * sizeof(double*));
    if (matrix == NULL) {
        write_message("Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < rows; i++) {
        matrix[i] = (double*)malloc(cols * sizeof(double));
        if (matrix[i] == NULL) {
            write_message("Error: Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
    return matrix;
}

void freeMatrix(double** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

void generateMatrix(double** matrix, int rows, int cols) {
    unsigned int seed = 12345; 
    double min = 0.0, max = 10.0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = min + (max - min) * ((double)rand_r(&seed) / (double)RAND_MAX); 
        }
    }
}

void writeMatrix(double** matrix, int rows, int cols) {
    char buffer[128];
    int length;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            length = snprintf(buffer, sizeof(buffer), "%5.1f ", matrix[i][j]);
            if (length < 0) {
                write_message("Error: Failed to format string\n");
                exit(EXIT_FAILURE);
            }
            if (write(STDOUT_FILENO, buffer, length) == -1) {
                write_message("Error: Write failed\n");
                exit(EXIT_FAILURE);
            }
        }
        if (write(STDOUT_FILENO, "\n", 1) == -1) {
            write_message("Error: Write failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

void dilation(double** input, double** output, int start_rows, int end_rows) {
    for (int i = start_rows; i < end_rows; i++) {
        for (int j = 0; j < COLS; j++) {
            double maxVal = input[i][j];

            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = i + di;
                    int nj = j + dj;
                    if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS) {
                        if (input[ni][nj] > maxVal) {
                            maxVal = input[ni][nj];
                        }
                    }
                }
            }
            output[i][j] = maxVal; 
        }
    }
}

void erosion(double** input, double** output, int start_rows, int end_rows) {
    for (int i = start_rows; i < end_rows; i++) {
        for (int j = 0; j < COLS; j++) {
            double minVal = input[i][j];

            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = i + di;
                    int nj = j + dj;
                    if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS) {
                        if (input[ni][nj] < minVal) {
                            minVal = input[ni][nj];
                        }
                    }
                }
            }
            output[i][j] = minVal; 
        }
    }
}

void *process_task(void *arg) {
    Task *task = (Task *)arg;

    erosion(task->matrix_erosion, task->result_erosion, task->start_row, task->end_row);
    dilation(task->matrix_dilation, task->result_dilation, task->start_row, task->end_row);

    pthread_mutex_lock(&thread_count_mutex);
    active_threads--; 
    pthread_cond_signal(&thread_available); 
    pthread_mutex_unlock(&thread_count_mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        write_message("Error: Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int max_threads = atoi(argv[1]);
    int K = atoi(argv[2]);

    if (max_threads < 1 || K < 1) {
        write_message("Error: max_threads and K must be > 0\n");
        exit(EXIT_FAILURE);
    }

    clock_t start = clock();

    pthread_mutex_init(&thread_count_mutex, NULL);
    pthread_cond_init(&thread_available, NULL);

    double** erosion_matrix = allocateMatrix(ROWS, COLS);
    double** erosion_result = allocateMatrix(ROWS, COLS);
    double** dilation_matrix = allocateMatrix(ROWS, COLS);
    double** dilation_result = allocateMatrix(ROWS, COLS);

    generateMatrix(erosion_matrix, ROWS, COLS);

    write_message("Input matrix:\n");
    writeMatrix(erosion_matrix, ROWS, COLS);

    for (int k = 0; k < K; k++) {
        max_threads = (max_threads > ROWS) ? ROWS : max_threads;
        pthread_t threads[max_threads];
        Task tasks[max_threads];
        int rows_per_thread = ROWS / max_threads;
        int task_count = 0;

        for (int i = 0; i < max_threads; i++) {
            pthread_mutex_lock(&thread_count_mutex);
            while (active_threads >= max_threads) {
                pthread_cond_wait(&thread_available, &thread_count_mutex); 
            }
            active_threads++;
            pthread_mutex_unlock(&thread_count_mutex);

            tasks[task_count].start_row = i * rows_per_thread;
            tasks[task_count].end_row = (i == max_threads - 1) ? ROWS : (i + 1) * rows_per_thread;
            tasks[task_count].matrix_erosion = (k == 0) ? erosion_matrix : erosion_result;
            tasks[task_count].result_erosion = erosion_result;
            
            tasks[task_count].matrix_dilation = (k == 0) ? erosion_matrix : dilation_result;
            tasks[task_count].result_dilation = dilation_result;

            pthread_create(&threads[task_count], NULL, process_task, &tasks[task_count]);
            task_count++;
        }

        for (int t = 0; t < task_count; t++) {
            pthread_join(threads[t], NULL);
        }

        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                erosion_matrix[i][j] = erosion_result[i][j];
                dilation_matrix[i][j] = dilation_result[i][j];
            }
        }
    }

    write_message("\nErosion matrix:\n");
    writeMatrix(erosion_result, ROWS, COLS);
    write_message("\nDilation matrix:\n");
    writeMatrix(dilation_result, ROWS, COLS);

    pthread_mutex_destroy(&thread_count_mutex);
    pthread_cond_destroy(&thread_available);

    freeMatrix(erosion_matrix, ROWS);
    freeMatrix(erosion_result, ROWS);
    freeMatrix(dilation_matrix, ROWS);
    freeMatrix(dilation_result, ROWS);

    clock_t end = clock();
    char time[64];
    float seconds = (float)(end - start) / CLOCKS_PER_SEC;
    snprintf(time, sizeof(time), "%lf\n", seconds);
    write_message(time);

    return 0;
}
