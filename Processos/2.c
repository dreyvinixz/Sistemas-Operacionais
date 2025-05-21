#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

// Definindo os padrões de paridade para o processo pai e filho
typedef enum {
    BOTH_EVEN,
    BOTH_ODD,
    PARENT_EVEN_CHILD_ODD,
    PARENT_ODD_CHILD_EVEN
} ParityPattern;

// Função para verificar se um número é par
int is_even(pid_t n) {
    return n % 2 == 0;
}

// Função para determinar o padrão de paridade entre o pai e o filho
ParityPattern get_parity_pattern(pid_t parent, pid_t child) {
    if (is_even(parent) && is_even(child)) {
        return BOTH_EVEN;
    } else if (!is_even(parent) && !is_even(child)) {
        return BOTH_ODD;
    } else if (is_even(parent) && !is_even(child)) {
        return PARENT_EVEN_CHILD_ODD;
    } else {
        return PARENT_ODD_CHILD_EVEN;
    }
}

// Função para obter o tamanho do vetor com validação de entrada
int get_array_size() {
    int size;
    do {
        printf("Digite o tamanho do vetor: ");
        if (scanf("%d", &size) != 1 || size <= 0) {
            printf("Entrada inválida! Por favor, digite um número inteiro positivo.\n");
            while (getchar() != '\n'); // Limpa o buffer de entrada
            size = 0;
        }
    } while (size <= 0);
    return size;
}

// Função para alocar memória para o vetor
int *allocate_memory(int size) {
    int *array = malloc(size * sizeof(int));
    if (!array) { // Verifica se a alocação falhou
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    } 
    return array;
}

// Função para gerar um vetor de números aleatórios
void generate_random_array(int *a, int size, const char *name) {
    for (int i = 0; i < size; i++) {
        a[i] = rand() % 100;
        printf("%s[%d] = %d\n", name, i, a[i]);
        sleep(2); 
    }
}

// Função para multiplicar dois vetores
void multiply_arrays(int *a, int *b, int *r, int n) {
    printf("Filho (pid %d) multiplicando vetores …\n", getpid());
    fflush(stdout);
    sleep(2);
    for (int i = 0; i < n; i++) {
        r[i] = a[i] * b[i];
        printf("array1[%d] * array2[%d] = %d * %d = %d\n", i, i, a[i], b[i], r[i]);
        sleep(2); 
    }
}

// Função para subtrair dois vetores
void subtract_arrays(int *a, int *b, int *r, int n) {
    printf("Filho (pid %d) subtraindo vetores …\n", getpid());
    fflush(stdout);
    sleep(2);
    for (int i = 0; i < n; i++) {
        r[i] = a[i] - b[i];
        printf("array1[%d] - array2[%d] = %d - %d = %d\n", i, i, a[i], b[i], r[i]);
        sleep(2); 
    }
}

// Função para somar dois vetores
void sum_arrays(int *a, int *b, int *r, int n) {
    printf("Filho (pid %d) somando vetores …\n", getpid());
    fflush(stdout);
    sleep(2);
    for (int i = 0; i < n; i++) {
        r[i] = a[i] + b[i];
        printf("array1[%d] + array2[%d] = %d + %d = %d\n", i, i, a[i], b[i], r[i]);
        sleep(2); 
    }
}

int main() {
    // Obtendo o tamanho do vetor do usuário
    int size = get_array_size();
    srand(time(NULL) ^ getpid()); // Inicializando o gerador de números aleatórios com uma semente única

    // Criando o processo filho
    pid_t child = fork();

    if (child != 0) {
        // Se for o processo pai
        int status;
        printf("\nProcesso Pai (pid %d) executando. Filho (pid %d) criado.\n", getpid(), child);
        printf("Estado: atual = %d, pai = %d, filho = %d\n", getpid(), getppid(), child);
        fflush(stdout);

        // Se o filho for par, o pai espera
        if (is_even(child)) {
            printf("Pai (pid %d) esperando Filho (pid %d)\n", getpid(), child);
            fflush(stdout);
            waitpid(child, &status, 0); // Espera o filho terminar
            printf("Filho (pid %d) terminou\n", child);
            fflush(stdout);
            sleep(2);
            printf("\n[Troca de contexto]: Controle retornou ao Pai (pid %d)\n", getpid());
            printf("Estado: atual = %d, pai = %d, filho = %d (finalizado)\n", getpid(), getppid(), 0);
            sleep(2);
            printf("Pai (pid %d) saiu.\n", getpid());
            printf("Programa finalizado.\n");
            fflush(stdout);
        } else {
            // Se o filho for ímpar, o pai não espera
            printf("Pai (pid %d) não espera Filho (pid %d)\n", getpid(), child);
            printf("Pai (pid %d) saiu.\n", getpid());
            fflush(stdout);
        }
        exit(0);
    } else {
        // Se for o processo filho
        pid_t parent_before = getppid(); // Pega o pid do processo pai antes da troca de contexto
        sleep(2);

        printf("\n[Troca de contexto]: Processo Filho (pid %d) começou.\n", getpid());
        printf("Estado: atual = %d, pai = %d, filho = %d\n", getpid(), getppid(), 0);
        fflush(stdout);

        // O filho gera vetores aleatórios
        printf("Filho (pid %d) gerando vetores aleatórios…\n", getpid());
        fflush(stdout);
        sleep(2);

        int *array1 = allocate_memory(size);
        int *array2 = allocate_memory(size);
        int *result = allocate_memory(size);

        generate_random_array(array1, size, "array1");
        generate_random_array(array2, size, "array2");

        // Realiza a operação de acordo com o padrão de paridade entre o pai e o filho
        switch (get_parity_pattern(parent_before, getpid())) {
        case BOTH_EVEN:
            multiply_arrays(array1, array2, result, size);
            break;
        case BOTH_ODD:
            subtract_arrays(array1, array2, result, size);
            break;
        case PARENT_ODD_CHILD_EVEN:
            sum_arrays(array1, array2, result, size);
            break;
        case PARENT_EVEN_CHILD_ODD:
            multiply_arrays(array1, array2, result, size);
            subtract_arrays(array1, array2, result, size);
            sum_arrays(array1, array2, result, size);
            break;
        }

        // Se o processo filho for ímpar, ele termina
        if (!is_even(getpid())) {
            printf("Filho (pid %d) saiu.\n", getpid());
            printf("Programa finalizado.\n");
        }

        // Liberando a memória alocada
        free(array1);
        free(array2);
        free(result);
        exit(0);
    }
    return 0;
}
