#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h> // Biblioteca para gerar os números aleatórios e contar o tempo de execução

// Estrutura para armazenar os dados de cada thread
typedef struct {
    int id;               // Identificador da thread
    int start;            // Índice inicial para a thread processar
    int end;              // Índice final para a thread processar
    int rows;             // Número de linhas da matriz
    int cols;             // Número de colunas da matriz
    int **matrix_A;       // Matriz A
    int **matrix_B;       // Matriz B
    int **matrix_C;       // Matriz C (resultado da soma)
} thread_data;

// Função para alocar uma matriz com o número de linhas e colunas fornecido
int **allocate_matrix(int rows, int cols) {
    int **matrix = (int **)malloc(rows * sizeof(int *));  // Aloca o vetor de ponteiros para as linhas
    for (int i = 0; i < rows; i++) {
        matrix[i] = (int *)malloc(cols * sizeof(int));  // Aloca a memória para cada linha
    }
    return matrix;
}

// Função para preencher a matriz com valores aleatórios
void fill_random_matrix(int rows, int cols, int **matrix) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = rand() % 100;  // Preenche com números aleatórios entre 0 e 99
        }
    }
}

// Função executada pelas threads para somar os elementos das matrizes A e B
void *sum_matrices(void *arg) {
    thread_data *data = (thread_data *)arg;  // Recebe os dados da thread

    // Cada thread soma os elementos da matriz dentro do intervalo específico
    for (int idx = data->start; idx < data->end; idx++) {
        int i = idx / data->cols;  // Calcula o índice da linha
        int j = idx % data->cols;  // Calcula o índice da coluna
        int a = data->matrix_A[i][j]; // Acessa o elemento da matriz A
        int b = data->matrix_B[i][j]; // Acessa o elemento da matriz B
        data->matrix_C[i][j] = a + b;  // Armazena a soma na matriz C
        printf("Thread %d soma elemento [%d,%d]: %d + %d = %d\n", data->id, i + 1, j + 1, a, b, a + b);
    }
    return NULL;
}

// Função para imprimir a matriz no console
void print_matrix(int rows, int cols, int **matrix) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%3d ", matrix[i][j]);  // Imprime os elementos de maneira formatada
        }
        printf("\n");
    }
}

// Função para obter o tamanho das matrizes a partir da entrada do usuário
void get_matrix_size(int *rows, int *cols) {
    // Loop até que o número de linhas seja válido
    do {
        printf("Digite o número de linhas: ");
        if (scanf("%d", rows) != 1 || *rows <= 0) {
            printf("Entrada inválida!\n");
            while (getchar() != '\n');  // Limpa o buffer de entrada
            *rows = 0;
        }
    } while (*rows <= 0);

    // Loop até que o número de colunas seja válido
    do {
        printf("Digite o número de colunas: ");
        if (scanf("%d", cols) != 1 || *cols <= 0) {
            printf("Entrada inválida!\n");
            while (getchar() != '\n');  // Limpa o buffer de entrada
            *cols = 0;
        }
    } while (*cols <= 0);
}

// Função para obter o número de threads que o usuário deseja usar
int get_threads_count(int max_threads) {
    int threads_count;
    do {
        printf("Digite o número de threads (até %d): ", max_threads);
        if (scanf("%d", &threads_count) != 1 || threads_count <= 0 || threads_count > max_threads) {
            printf("Entrada inválida!\n");
            while (getchar() != '\n');  // Limpa o buffer de entrada
            threads_count = 0;
        }
    } while (threads_count <= 0);
    return threads_count;
}

int main() {
    srand(time(NULL));  // Semente para gerar números aleatórios baseados no tempo
    int rows, cols;

    get_matrix_size(&rows, &cols);  // Obtém o tamanho das matrizes

    // Aloca memória para as três matrizes (A, B e C)
    int **matrix_A = allocate_matrix(rows, cols);
    int **matrix_B = allocate_matrix(rows, cols);
    int **matrix_C = allocate_matrix(rows, cols);

    // Preenche as matrizes A e B com números aleatórios
    fill_random_matrix(rows, cols, matrix_A);
    fill_random_matrix(rows, cols, matrix_B);

    printf("\nMatriz A:\n");
    print_matrix(rows, cols, matrix_A);  // Imprime a matriz A

    printf("\nMatriz B:\n");
    print_matrix(rows, cols, matrix_B);  // Imprime a matriz B

    int num_elements = rows * cols;  // Calcula o número total de elementos na matriz
    int threads_count = get_threads_count(num_elements);  // Obtém o número de threads

    pthread_t threads[threads_count];  // Cria um vetor para as threads
    thread_data data[threads_count];   // Cria um vetor para armazenar os dados das threads

    int base_load = num_elements / threads_count;  // Calcula a carga base (elementos por thread)
    int remainder = num_elements % threads_count;  // Calcula o restante para balancear a carga

    clock_t start_time = clock();  // Inicia o contador de tempo

    // Cria as threads e distribui o trabalho entre elas
    for (int i = 0; i < threads_count; i++) {
        data[i].id = i;  // Define o ID da thread
        if (i < remainder) {
            data[i].start = i * base_load + i;  // Ajusta o intervalo de trabalho para threads extras
        } else {
            data[i].start = i * base_load + remainder;
        }
        if (i < remainder) {
            data[i].end = data[i].start + base_load + 1;
        } else {
            data[i].end = data[i].start + base_load;
        }
        data[i].rows = rows;  // Passa o número de linhas para os dados
        data[i].cols = cols;  // Passa o número de colunas para os dados
        data[i].matrix_A = matrix_A;  // Passa a matriz A
        data[i].matrix_B = matrix_B;  // Passa a matriz B
        data[i].matrix_C = matrix_C;  // Passa a matriz C (onde o resultado será armazenado)

        pthread_create(&threads[i], NULL, sum_matrices, (void *)&data[i]);  // Cria a thread
        printf("Thread %d -> intervalo [%d, %d)\n", data[i].id, data[i].start + 1, data[i].end + 1);  // Mostra o intervalo de cada thread
    }

    // Sincroniza as threads, aguardando sua finalização
    for (int j = 0; j < threads_count; j++) {
        pthread_join(threads[j], NULL);
    }

    clock_t end_time = clock();  // Finaliza o contador de tempo
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;  // Calcula o tempo total de execução

    printf("\nMatriz C (A + B):\n");
    print_matrix(rows, cols, matrix_C);  // Imprime a matriz resultante C

    printf("Tempo total: %f segundos\n", elapsed_time);  // Imprime o tempo de execução total

    return 0;
}
