# README - Diretório `Threads`

Este diretório faz parte do repositório de Sistemas Operacionais e contém implementações e exemplos relacionados ao conceito de threads.

## Estrutura dos Arquivos

- **`main.c`**  
    Código-fonte principal que demonstra a criação, execução e sincronização de threads em C, utilizando a biblioteca POSIX Threads (`pthread`).  
    - Criação de múltiplas threads.
    - Uso de funções de callback para execução concorrente.
    - Sincronização com `pthread_join`.
    - Possível uso de mutexes para evitar condições de corrida.

- **`Makefile`**  
    Arquivo para automação da compilação do projeto.  
    - Comandos para compilar o `main.c` com as flags corretas (`-lpthread`).
    - Alvos comuns: `all`, `clean`.

- **`README.md`**  
    Este documento, com instruções de uso, descrição dos arquivos e informações técnicas.

## Como Compilar

No terminal, execute:
```sh
make
```
Ou, manualmente:
```sh
gcc main.c -o threads -lpthread
```

## Como Executar

Após a compilação:
```sh
./threads
```

## Requisitos

- GCC (compilador C)
- Biblioteca POSIX Threads (`pthread`)

## Funcionalidades Demonstradas

- Criação e gerenciamento de threads.
- Sincronização de threads.
- Compartilhamento de dados entre threads.
- Prevenção de condições de corrida (se aplicável).

## Observações

- O código serve como base para estudos de programação concorrente em Sistemas Operacionais.
- Para expandir, adicione exemplos de uso de semáforos, variáveis de condição ou outros mecanismos de sincronização.

---

Para dúvidas ou sugestões, abra uma issue no repositório.