#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    int *A, *B, *C;
    int m, n, p;
    int start_row, end_row;
} ThreadData;

void *multiply(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = data->start_row; i < data->end_row; ++i) {
        for (int j = 0; j < data->p; ++j) {
            int sum = 0;
            for (int k = 0; k < data->n; ++k) {
                sum += data->A[i * data->n + k] * data->B[k * data->p + j];
            }
            data->C[i * data->p + j] = sum;
        }
    }
    return NULL;
}

int *allocate_matrix(int rows, int cols) {
    return (int *)malloc(rows * cols * sizeof(int));
}

void fill_matrix(int *matrix, int rows, int cols) {
    for (int i = 0; i < rows * cols; ++i)
        matrix[i] = rand() % 10;
}

void print_matrix(int *matrix, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j)
            printf("%4d ", matrix[i * cols + j]);
        printf("\n");
    }
}

int main() {
    int m, n, p, num_threads;
    srand(time(NULL));

    printf("Digite as dimensões das matrizes A (MxN) e B (NxP):\n");
    printf("M: "); scanf("%d", &m);
    printf("N: "); scanf("%d", &n);
    printf("P: "); scanf("%d", &p);
    printf("Digite o número de threads: ");
    scanf("%d", &num_threads);

    int *A = allocate_matrix(m, n);
    int *B = allocate_matrix(n, p);
    int *C = allocate_matrix(m, p);

    fill_matrix(A, m, n);
    fill_matrix(B, n, p);

    printf("\nMatriz A:\n");
    print_matrix(A, m, n);

    printf("\nMatriz B:\n");
    print_matrix(B, n, p);

    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    int rows_per_thread = m / num_threads;
    int remaining_rows = m % num_threads;
    int current_row = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_threads; ++i) {
        int rows = rows_per_thread + (i < remaining_rows ? 1 : 0);

        thread_data[i] = (ThreadData){
            .A = A, .B = B, .C = C,
            .m = m, .n = n, .p = p,
            .start_row = current_row,
            .end_row = current_row + rows
        };

        pthread_create(&threads[i], NULL, multiply, &thread_data[i]);
        current_row += rows;
    }

    for (int i = 0; i < num_threads; ++i)
        pthread_join(threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\nMatriz Resultado (A x B):\n");
    print_matrix(C, m, p);

    printf("\nTempo de execução: %.6f segundos\n", elapsed);

    free(A); free(B); free(C);
    return 0;
}
