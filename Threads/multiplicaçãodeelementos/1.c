#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h> 

typedef struct {
    int tamanho;
    int num_threads;
    int carga;
    int resto;
    int inico;
    int fim;
    int limite;
    int *vetor_a;
    int *vetor_b;
    int *vetor_c;
} Parametros;

// função que preenche com numeros aleatorios
void preenche(int vetor_a[], int vetor_b[], int tamanho, int limite){
    srand(time(NULL));
    for(int i = 0; i < tamanho; i++){
        vetor_a[i] = rand() % tamanho;
        vetor_b[i] = rand() % tamanho;
    }
}

// função que calcula a carga para dividir o array
int carga(unsigned int tam_vetor, unsigned int num_num_threads){
    int carga = tam_vetor / num_num_threads;
    return carga;
}

int resto_carga(unsigned int tam_vetor, unsigned int num_num_threads){
    int resto_carga = tam_vetor % num_num_threads;
    return resto_carga;
}

// calcula o produto vetorial
void *multiplica(void *arg) {
    Parametros *p = (Parametros *)arg;

    for(int i = p->inico; i < p->fim; i++) {
        p->vetor_c[i] = p->vetor_a[i] * p->vetor_b[i];
    }

    pthread_exit(NULL);
}

void print_vetores(const char *nome, int vetor[], int tamanho){
    printf("\n%s:\n", nome);
    for (int i = 0; i < tamanho; i++) {
        printf("%d ", vetor[i]);
    }
    printf("\n");
}

int main(){

    Parametros *parametros = malloc(sizeof(Parametros));

    int total = 0; // contador usado para mostrar no terminal o total de threads e a sequência 

    parametros->limite = 100;
    int num_threads_e;
    int tamanho_e;

    printf("Entre com um numero de threads: ");
    scanf("%d", &num_threads_e);

    printf("Entre com o tamanho do vetor: ");
    scanf("%d", &tamanho_e);

    // verifica se o usuario entro com um valor errado
    while(num_threads_e > tamanho_e || num_threads_e == 0){
        printf("Favor nao entrar com um numero de threads > que tamanho ou = 0\n");
        scanf("%d", &num_threads_e);
    }  

    parametros->tamanho =  tamanho_e;
    parametros->num_threads = num_threads_e;

    int valor_carga = carga(parametros->tamanho, parametros->num_threads);
    parametros->carga = valor_carga;

    int valor_resto_carga = resto_carga(parametros->tamanho, parametros->num_threads);
    parametros->resto = valor_resto_carga;

    // vetores que armazenam os dados
    int vetor_a [parametros->tamanho];
    int vetor_b [parametros->tamanho];
    int vetor_c [parametros->tamanho];

    // chamndo minha função para preencher os verores
    preenche(vetor_a, vetor_b, parametros->tamanho, parametros->limite);

    // alocando as threads
    pthread_t threads[parametros->num_threads];
    
    // for que executa os dados
    for(int i = 0; i < parametros->num_threads; i++){
        
        Parametros *args = malloc(sizeof(Parametros));
        *args = *parametros; // copia os dados base  

        // logica que faz com que nas primeiras threads se processe mais 
        if (i < parametros->resto) {
            args->inico = i * valor_carga + i;
            args->fim = args->inico + valor_carga + 1;
        } else {
            args->inico = i * valor_carga + parametros->resto;
            args->fim = args->inico + valor_carga;
        }

        args->vetor_a = vetor_a;
        args->vetor_b = vetor_b;
        args->vetor_c = vetor_c;
    
        pthread_create(&threads[i], NULL, multiplica, (void *)args);
        total++;
        printf("\nThread %d - Numero ao todo %d", total, num_threads_e);
    }
    
    //sicronização
    for(int i = 0; i < parametros->num_threads; i++ ){
        pthread_join(threads[i], NULL);
    }

    // Impressão dos vetores
    print_vetores("Vetor_A", vetor_a, parametros->tamanho);
    print_vetores("Vetor_B", vetor_b, parametros->tamanho);
    print_vetores("Vetor_C", vetor_c, parametros->tamanho);

    free(parametros);
    return 0; 
}