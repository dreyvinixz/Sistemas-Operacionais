#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Estrutura para armazenar os dados que cada thread vai usar
typedef struct {
    int id;                 // ID da thread
    int m, n;               // Dimensões da matriz original (m linhas, n colunas)
    int total_threads;      // Total de threads
    int **A, **B;           // Ponteiros para a matriz original (A) e transposta (B)
} ThreadData;

// Função executada por cada thread para transpor parte da matriz
void *transpor_varios_elementos(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int id = data->id;
    int m = data->m;
    int n = data->n;
    int total = m * n;

    // Cada thread percorre os elementos que lhe cabem com passo igual ao número total de threads
    for (int idx = id; idx < total; idx += data->total_threads) {
        int i = idx / n;  // linha original
        int j = idx % n;  // coluna original

        // A transposição troca linhas por colunas
        data->B[j][i] = data->A[i][j];

        printf("Thread %d (ID: %lu) transposicionou A[%d][%d] = %d → B[%d][%d]\n",
               id, pthread_self(), i, j, data->A[i][j], j, i);
    }

    return NULL;
}

// Função para alocar memória para uma matriz m x n
int **alocar_matriz(int rows, int cols) {
    int **mat = malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++)
        mat[i] = malloc(cols * sizeof(int));
    return mat;
}

// Libera a memória alocada para a matriz
void liberar_matriz(int **mat, int rows) {
    for (int i = 0; i < rows; i++)
        free(mat[i]);
    free(mat);
}

// Imprime uma matriz na tela
void imprimir_matriz(int **mat, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++)
            printf("%3d ", mat[i][j]);
        printf("\n");
    }
}

int main() {
    int m, n, num_threads;

    // Leitura das dimensões da matriz
    printf("Digite o número de linhas (M): ");
    scanf("%d", &m);
    printf("Digite o número de colunas (N): ");
    scanf("%d", &n);

    // Cálculo do número total de elementos
    int total_elementos = m * n;
    printf("Número máximo de threads permitidas: %d\n", total_elementos);
    printf("Digite o número de threads (1 a %d): ", total_elementos);
    scanf("%d", &num_threads);

    // Verificação de validade do número de threads
    if (num_threads < 1 || num_threads > total_elementos) {
        printf("Número de threads inválido!\n");
        return 1;
    }

    // Alocação das matrizes
    int **A = alocar_matriz(m, n);
    int **B = alocar_matriz(n, m); // B é a transposta, então dimensões invertidas

    // Preenchimento aleatório da matriz A
    srand(time(NULL));
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            A[i][j] = rand() % 100;

    // Exibe a matriz original
    printf("\nMatriz A:\n");
    imprimir_matriz(A, m, n);

    // Criação das threads e estrutura de dados associada
    pthread_t threads[num_threads];
    ThreadData dados[num_threads];

    for (int t = 0; t < num_threads; t++) {
        dados[t] = (ThreadData){
            .id = t,
            .m = m,
            .n = n,
            .total_threads = num_threads,
            .A = A,
            .B = B
        };
        pthread_create(&threads[t], NULL, transpor_varios_elementos, &dados[t]);
    }

    // Espera todas as threads terminarem
    for (int t = 0; t < num_threads; t++)
        pthread_join(threads[t], NULL);

    // Exibe a matriz transposta
    printf("\nMatriz B (Transposta):\n");
    imprimir_matriz(B, n, m);

    // Libera memória
    liberar_matriz(A, m);
    liberar_matriz(B, n);

    return 0;
}
