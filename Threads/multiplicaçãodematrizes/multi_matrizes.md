# Documentação Técnica - Multiplicação de Matrizes com Threads

## 1. Bibliotecas Utilizadas

```c
#include <stdio.h>     // Funções de entrada e saída, como printf e scanf.
#include <stdlib.h>    // Funções de alocação dinâmica (malloc), geração de números aleatórios (rand), etc.
#include <pthread.h>   // Biblioteca POSIX Threads, usada para manipular múltiplas threads.
#include <time.h>      // Funções relacionadas a tempo, como medir o tempo de execução (clock_gettime).
```

## 2. Definição da Estrutura `ThreadData`

```c
typedef struct {
    int *A, *B, *C;          // Ponteiros para as matrizes A, B e C.
    int m, n, p;             // Dimensões: A é m x n, B é n x p, logo C é m x p.
    int start_row, end_row; // Faixa de linhas da matriz C que a thread será responsável por calcular.
} ThreadData;
```

Essa estrutura agrupa todos os dados que cada thread precisará acessar para realizar a multiplicação de uma parte da matriz.

## 3. Função `multiply` (executada por cada thread)

```c
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
```

Explicação: Cada thread executa a multiplicação apenas para as linhas da matriz C que lhe foram atribuídas (`start_row` até `end_row`).

A fórmula clássica da multiplicação de matrizes é aplicada:

```
C[i][j] = soma(A[i][k] * B[k][j]), para k = 0 até n-1
```

## 4. Funções Auxiliares

### `allocate_matrix`
```c
int *allocate_matrix(int rows, int cols) {
    return (int *)malloc(rows * cols * sizeof(int));
}
```

Aloca dinamicamente um vetor linear que representará uma matriz bidimensional de inteiros.

### `fill_matrix`
```c
void fill_matrix(int *matrix, int rows, int cols) {
    for (int i = 0; i < rows * cols; ++i)
        matrix[i] = rand() % 10;
}
```

Preenche a matriz com valores aleatórios entre 0 e 9.

### `print_matrix`
```c
void print_matrix(int *matrix, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j)
            printf("%4d ", matrix[i * cols + j]);
        printf("\n");
    }
}
```

Imprime a matriz em formato legível (linha por linha).

## 5. Função Principal `main`

### a. Leitura das Dimensões e Número de Threads

```c
printf("M: "); scanf("%d", &m);
printf("N: "); scanf("%d", &n);
printf("P: "); scanf("%d", &p);
printf("Digite o número de threads: ");
scanf("%d", &num_threads);
```

### b. Alocação Dinâmica de Memória

```c
int *A = allocate_matrix(m, n);
int *B = allocate_matrix(n, p);
int *C = allocate_matrix(m, p);
```

### c. Preenchimento e Impressão das Matrizes A e B

```c
fill_matrix(A, m, n);
fill_matrix(B, n, p);
print_matrix(A, m, n);
print_matrix(B, n, p);
```

### d. Criação e Distribuição de Threads

```c
pthread_t threads[num_threads];
ThreadData thread_data[num_threads];

int rows_per_thread = m / num_threads;
int remaining_rows = m % num_threads;
int current_row = 0;

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
```

### e. Sincronização com `pthread_join`

```c
for (int i = 0; i < num_threads; ++i)
    pthread_join(threads[i], NULL);
```

### f. Medição de Tempo de Execução

```c
struct timespec start, end;
clock_gettime(CLOCK_MONOTONIC, &start);
// execução das threads
clock_gettime(CLOCK_MONOTONIC, &end);
```

### g. Impressão da Matriz Resultante

```c
print_matrix(C, m, p);
```

### h. Liberação de Memória

```c
free(A); free(B); free(C);
```

## 6. Resumo do Funcionamento das Threads

- **Onde são criadas?** Dentro da `main`, com `pthread_create`.
- **O que fazem?** Executam `multiply`, calculando subconjuntos da matriz `C`.
- **Como são sincronizadas?** Com `pthread_join`.

## 7. Lógica de Multiplicação de Matrizes

```c
C[i * p + j] = soma(A[i * n + k] * B[k * p + j]) para k de 0 até n-1
```

## 8. Chamadas Ocultas e Comportamentos Implícitos

- `pthread_create`, `pthread_join` e `malloc` fazem chamadas ao sistema operacional.
- `rand()` depende de `srand(time(NULL))` para valores variados.
- `clock_gettime` acessa temporizadores de alta resolução.
