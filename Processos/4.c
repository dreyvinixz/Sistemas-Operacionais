#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Função para obter o caminho do programa a ser executado pelo processo filho
void get_path(char *path) {
    printf("Digite o caminho do programa que o processo filho deve executar:\n");
    scanf("%255s", path);  // Lê uma string do usuário com no máximo 255 caracteres
}

int main() {
    char path[256];  // vetor para armazenar o caminho
    get_path(path);  // Solicita e lê o caminho do programa

    pid_t pid = fork();  // Cria um novo processo

    if (pid < 0) {
        // Erro ao criar processo
        perror("Erro ao criar o processo filho");
        return 1;
    }

    if (pid == 0) {
        // Processo filho
        printf("Filho (PID %d): executando '%s'\n", getpid(), path);
        sleep(2);  // Espera 2 segundos
        execl(path, path, (char *)NULL);  // Executa o programa indicado
        perror("execl falhou");  // Só executa se execl falhar
        exit(1);  // Sai com erro
    } else {
        // Processo pai
        int status;
        printf("Pai (PID %d): esperando o filho (PID %d) terminar...\n", getpid(), pid);
        sleep(2);  // Espera 2 segundos
        waitpid(pid, &status, 0);  // Espera o processo filho terminar
        printf("Pai (PID %d): processo filho terminou\n", getpid());
    }
    return 0;
}
