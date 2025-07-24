/*
 * SISTEMA BANCÁRIO COM SIMULAÇÃO DE CONCORRÊNCIA
 * ============================================
 * 
 * Este programa simula um sistema bancário que permite operações concorrentes
 * (múltiplas threads executando simultaneamente) sobre contas correntes.
 * 
 * PRINCIPAIS CONCEITOS DEMONSTRADOS:
 * - Sincronização de threads com mutex e shared_mutex
 * - Padrão Reader-Writer (múltiplos leitores, um escritor)
 * - Operações atômicas para contadores
 * - Simulação de carga de trabalho concorrente
 * - Logging e análise de performance
 */



 
#include <iostream>          // Para entrada/saída (cout, cin)
#include <fstream>           // Para manipulação de arquivos
#include <thread>            // Para criação e gerenciamento de threads
#include <mutex>             // Para sincronização básica (exclusão mútua)
#include <condition_variable> // Para sincronização avançada (não usado neste código)
#include <shared_mutex>      // Para Reader-Writer locks
#include <vector>            // Para arrays dinâmicos
#include <map>               // Para mapeamento chave-valor
#include <random>            // Para geração de números aleatórios
#include <chrono>            // Para medição de tempo
#include <string>            // Para manipulação de strings
#include <sstream>           // Para conversão string-stream
#include <atomic>            // Para operações atômicas (thread-safe)
#include <iomanip>           // Para formatação de saída

// ===================================
// Classe ContaCorrente
// ===================================
class ContaCorrente {
private:
    std::string identificador;              // ID único da conta
    double saldo;                          // Saldo atual da conta
    
    /*
     * SINCRONIZAÇÃO READER-WRITER:
     * - shared_mutex permite múltiplos leitores OU um escritor
     * - Consultas (leitura) podem acontecer simultaneamente
     * - Débitos/créditos (escrita) são exclusivos
     */
    mutable std::shared_mutex rwMutex;     // Mutex para Reader-Writer pattern
    
    /*
     * CONTADOR ATÔMICO:
     * - atomic<int> garante que operações de incremento/decremento sejam thread-safe
     * - Usado para limitar o número de consultas simultâneas
     */
    mutable std::atomic<int> leitoresAtivos{0};  // Conta quantos threads estão lendo

public:
    /*
     * CONSTRUTOR:
     * Inicializa a conta com ID e saldo inicial
     */
    ContaCorrente(const std::string& id, double saldoInicial) 
        : identificador(id), saldo(saldoInicial) {}

    /*
     * GETTER SIMPLES:
     * Retorna o ID da conta (operação rápida, não precisa de sincronização)
     */
    std::string getId() const { return identificador; }

    /*
     * OPERAÇÃO DE CRÉDITO (ESCRITA):
     * Adiciona dinheiro à conta de forma thread-safe
     */
    bool creditar(double valor) {
        /*
         * UNIQUE_LOCK:
         * - Bloqueia EXCLUSIVAMENTE o shared_mutex
         * - Nenhuma outra thread pode ler ou escrever enquanto isso
         * - Garante que apenas uma thread modifique o saldo por vez
         */
        std::unique_lock<std::shared_mutex> lock(rwMutex);
        
        // Validação: valor deve ser positivo
        if (valor <= 0) return false;
        
        // Operação crítica: modificação do saldo
        saldo += valor;
        
        // Log da operação (formatação com 2 casas decimais)
        std::cout << "[CRÉDITO] Conta " << identificador 
                  << " - Valor: R$ " << std::fixed << std::setprecision(2) << valor
                  << " - Novo saldo: R$ " << saldo << std::endl;
        
        /*
         * SIMULAÇÃO DE PROCESSAMENTO:
         * Sleep simula o tempo que uma operação bancária real levaria
         * (acesso ao banco de dados, validações, etc.)
         */
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        return true;  // Operação bem-sucedida
    }

    /*
     * OPERAÇÃO DE DÉBITO (ESCRITA):
     * Remove dinheiro da conta de forma thread-safe
     */
    bool debitar(double valor) {
        // Mesmo padrão do crédito: lock exclusivo
        std::unique_lock<std::shared_mutex> lock(rwMutex);
        
        // Validação: valor positivo e saldo suficiente
        if (valor <= 0 || saldo < valor) return false;
        
        // Operação crítica: modificação do saldo
        saldo -= valor;
        
        // Log da operação
        std::cout << "[DÉBITO] Conta " << identificador 
                  << " - Valor: R$ " << std::fixed << std::setprecision(2) << valor
                  << " - Novo saldo: R$ " << saldo << std::endl;
        
        // Simulação de processamento
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        return true;
    }

    /*
     * CONSULTA DE SALDO (LEITURA):
     * Permite múltiplas threads lerem simultaneamente
     */
    double consultarSaldo() const {
        /*
         * SHARED_LOCK:
         * - Permite que múltiplas threads leiam ao mesmo tempo
         * - Bloqueia apenas se houver um escritor ativo
         * - Mais eficiente que unique_lock para operações de leitura
         */
        std::shared_lock<std::shared_mutex> lock(rwMutex);
        
        /*
         * CONTROLE DE CONCORRÊNCIA:
         * Limita a 5 leitores simultâneos para evitar sobrecarga
         * load() é uma operação atômica que lê o valor atual
         */
        if (leitoresAtivos.load() >= 5) return -1;  // Muitos leitores ativos
        
        /*
         * INCREMENTO ATÔMICO:
         * ++ em atomic<int> é thread-safe (não precisa de mutex adicional)
         */
        leitoresAtivos++;
        
        // Log da consulta
        std::cout << "[CONSULTA] Conta " << identificador 
                  << " - Saldo: R$ " << std::fixed << std::setprecision(2) << saldo
                  << " - Leitores ativos: " << leitoresAtivos.load() << std::endl;
        
        // Simulação de processamento (mais rápida que escrita)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        // Cópia do saldo para retorno
        double saldoAtual = saldo;
        
        /*
         * DECREMENTO ATÔMICO:
         * Remove este thread da contagem de leitores ativos
         */
        leitoresAtivos--;
        
        return saldoAtual;
    }

    /*
     * ACESSO DIRETO (SEM SINCRONIZAÇÃO):
     * Usado apenas quando já há garantia de exclusão mútua
     * (ex: quando o banco já está com lock)
     */
    double getSaldoUnsafe() const {
        return saldo;
    }
};

// ===================================
// Classe Banco
// ===================================
class Banco {
private:
    /*
     * COLEÇÃO DE CONTAS:
     * - map<string, unique_ptr<ContaCorrente>>: mapeia ID -> ponteiro para conta
     * - unique_ptr garante gerenciamento automático de memória
     * - map mantém as contas ordenadas por ID
     */
    std::map<std::string, std::unique_ptr<ContaCorrente>> contas;
    
    /*
     * MUTEX PARA PROTEÇÃO DO MAP:
     * Protege a estrutura do map contra modificações concorrentes
     * (adição/remoção de contas)
     */
    std::mutex contasMutex;
    
    /*
     * CONTADORES ATÔMICOS:
     * Mantêm estatísticas globais sem necessidade de sincronização
     */
    std::atomic<int> operacoesRealizadas{0};   // Contador de operações bem-sucedidas
    std::atomic<int> operacoesFalhas{0};       // Contador de operações falhadas

public:
    /*
     * CARREGAMENTO DE CONTAS DO ARQUIVO:
     * Lê o arquivo e popula o map de contas
     */
    void carregarContas(const std::string& arquivo) {
        std::ifstream file(arquivo);
        if (!file.is_open()) {
            throw std::runtime_error("Arquivo ContaCorrente.txt não encontrado.");
        }

        std::string linha;
        /*
         * LEITURA LINHA POR LINHA:
         * Cada linha tem formato: ID|SALDO
         */
        while (std::getline(file, linha)) {
            std::stringstream ss(linha);
            std::string id;
            double saldo;
            
            /*
             * PARSING DA LINHA:
             * - getline(ss, id, '|'): lê até encontrar '|'
             * - ss >> saldo: lê o valor numérico
             */
            if (std::getline(ss, id, '|') && ss >> saldo) {
                /*
                 * PROTEÇÃO DO MAP:
                 * Lock exclusivo para modificar a coleção de contas
                 */
                std::lock_guard<std::mutex> lock(contasMutex);
                
                /*
                 * CRIAÇÃO DA CONTA:
                 * make_unique cria um novo objeto ContaCorrente
                 * e retorna um unique_ptr para ele
                 */
                contas[id] = std::make_unique<ContaCorrente>(id, saldo);
            }
        }

        std::cout << "Carregadas " << contas.size() << " contas do arquivo." << std::endl;
    }

    /*
     * SALVAMENTO DE CONTAS NO ARQUIVO:
     * Escreve o estado atual de todas as contas
     */
    void salvarContas(const std::string& arquivo) {
        std::ofstream file(arquivo);
        if (!file.is_open()) {
            std::cerr << "Erro ao salvar no arquivo: " << arquivo << std::endl;
            return;
        }

        /*
         * PROTEÇÃO PARA LEITURA:
         * Lock durante a iteração para evitar modificações concorrentes
         */
        std::lock_guard<std::mutex> lock(contasMutex);
        
        /*
         * ITERAÇÃO ESTRUTURADA (C++17):
         * for (const auto& [id, conta] : contas)
         * Desempacota automaticamente o par<string, unique_ptr>
         */
        for (const auto& [id, conta] : contas) {
            file << id << "|" << std::fixed << std::setprecision(2) 
                 << conta->getSaldoUnsafe() << std::endl;
        }

        std::cout << "Contas salvas no arquivo: " << arquivo << std::endl;
    }

    /*
     * OBTENÇÃO DE PONTEIRO PARA CONTA:
     * Retorna ponteiro raw para a conta (ou nullptr se não encontrar)
     */
    ContaCorrente* obterConta(const std::string& id) {
        std::lock_guard<std::mutex> lock(contasMutex);
        auto it = contas.find(id);
        return (it != contas.end()) ? it->second.get() : nullptr;
    }

    /*
     * LISTAGEM DE IDs:
     * Retorna vetor com todos os IDs das contas
     */
    std::vector<std::string> listarContas() {
        std::lock_guard<std::mutex> lock(contasMutex);
        std::vector<std::string> ids;
        for (const auto& [id, conta] : contas) {
            ids.push_back(id);
        }
        return ids;
    }

    /*
     * MÉTODOS PARA ESTATÍSTICAS:
     * Operações atômicas - não precisam de mutex
     */
    void incrementarOperacoes() { operacoesRealizadas++; }
    void incrementarFalhas() { operacoesFalhas++; }

    int getOperacoesRealizadas() const { return operacoesRealizadas.load(); }
    int getOperacoesFalhas() const { return operacoesFalhas.load(); }

    /*
     * RESET DE ESTATÍSTICAS:
     * store() é uma operação atômica para definir novo valor
     */
    void resetarEstatisticas() {
        operacoesRealizadas.store(0);
        operacoesFalhas.store(0);
    }

    /*
     * RELATÓRIO DE ESTATÍSTICAS:
     * Mostra resultados da simulação
     */
    void imprimirEstatisticas() {
        std::cout << "\n=== ESTATÍSTICAS FINAIS ===" << std::endl;
        std::cout << "Operações realizadas: " << operacoesRealizadas.load() << std::endl;
        std::cout << "Operações com falha: " << operacoesFalhas.load() << std::endl;
        std::cout << "Total de tentativas: " << (operacoesRealizadas + operacoesFalhas) << std::endl;

        std::cout << "\n=== SALDOS FINAIS ===" << std::endl;
        std::lock_guard<std::mutex> lock(contasMutex);
        for (const auto& [id, conta] : contas) {
            std::cout << "Conta " << id << ": R$ " << std::fixed << std::setprecision(2) 
                      << conta->getSaldoUnsafe() << std::endl;
        }
    }
};

// ===================================
// Classe Simulador de Operações
// ===================================
class SimuladorOperacoes {
private:
    Banco& banco;                                    // Referência ao banco
    std::mt19937 rng;                               // Gerador de números aleatórios
    std::uniform_int_distribution<> operacaoDist;   // Distribuição para tipo de operação (0-2)
    std::uniform_real_distribution<> valorDist;     // Distribuição para valores (10-500)
    std::atomic<bool> executando{true};             // Flag para parar execução

public:
    /*
     * CONSTRUTOR:
     * Inicializa o gerador aleatório e as distribuições
     */
    SimuladorOperacoes(Banco& b) : banco(b), 
                                   rng(std::random_device{}()),      // Seed aleatória
                                   operacaoDist(0, 2),              // 0=crédito, 1=débito, 2=consulta
                                   valorDist(10.0, 500.0) {}        // Valores entre R$ 10 e R$ 500

    /*
     * EXECUÇÃO DE OPERAÇÕES POR THREAD:
     * Cada thread executa um número determinado de operações aleatórias
     */
    void executarOperacoes(int threadId, int numOperacoes) {
        /*
         * OBTENÇÃO DA LISTA DE CONTAS:
         * Cada thread obtém uma cópia da lista de IDs
         */
        auto contas = banco.listarContas();
        if (contas.empty()) return;

        /*
         * DISTRIBUIÇÃO PARA SELEÇÃO DE CONTA:
         * Gera índices aleatórios para selecionar contas
         */
        std::uniform_int_distribution<> contaDist(0, contas.size() - 1);

        /*
         * LOOP PRINCIPAL DE OPERAÇÕES:
         * Cada thread executa o número especificado de operações
         */
        for (int i = 0; i < numOperacoes && executando; ++i) {
            /*
             * SELEÇÃO ALEATÓRIA DE CONTA:
             */
            std::string contaId = contas[contaDist(rng)];
            ContaCorrente* conta = banco.obterConta(contaId);
            
            if (!conta) {
                banco.incrementarFalhas();
                continue;
            }

            /*
             * SELEÇÃO ALEATÓRIA DE OPERAÇÃO:
             * 0 = crédito, 1 = débito, 2 = consulta
             */
            int operacao = operacaoDist(rng);
            double valor = valorDist(rng);
            bool sucesso = false;

            /*
             * EXECUÇÃO DA OPERAÇÃO:
             * Switch baseado no tipo de operação sorteado
             */
            switch (operacao) {
                case 0: sucesso = conta->creditar(valor); break;
                case 1: sucesso = conta->debitar(valor); break;
                case 2: sucesso = (conta->consultarSaldo() >= 0); break;
            }

            /*
             * ATUALIZAÇÃO DE ESTATÍSTICAS:
             * Incrementa contador apropriado baseado no resultado
             */
            sucesso ? banco.incrementarOperacoes() : banco.incrementarFalhas();
            
            /*
             * PAUSA ENTRE OPERAÇÕES:
             * Simula tempo de processamento entre operações
             */
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        std::cout << "Thread " << threadId << " finalizou suas operações.\n";
    }

    /*
     * PARADA SEGURA:
     * Sinaliza para todas as threads pararem
     */
    void pararExecucao() {
        executando.store(false);
    }
};

// ================================================================================================
// CLASSE LOGGER PARA SIMULAÇÕES
// ================================================================================================
/*
 * Responsável por registrar os resultados das simulações em arquivo CSV
 * para análise posterior de performance.
 */
class LoggerSimulacao {
private:
    std::mutex logMutex;        // Proteção para escrita no arquivo
    std::string nomeArquivo;    // Nome do arquivo de log
    bool headerEscrito;         // Flag para controle do cabeçalho

public:
    LoggerSimulacao(const std::string& arquivo) : nomeArquivo(arquivo), headerEscrito(false) {}

    /*
     * REGISTRO DE LOG DE SIMULAÇÃO:
     * Salva métricas da simulação em formato CSV
     */
    void registrarLogSimulacao(int numThreads, int operacoesPorThread, 
                              long long tempoExecucao, int operacoesSucesso, 
                              int operacoesFalhas) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        /*
         * VERIFICAÇÃO DE ARQUIVO EXISTENTE:
         * Determina se precisa escrever o cabeçalho CSV
         */
        std::ifstream teste(nomeArquivo);
        bool arquivoExiste = teste.good();
        teste.close();
        
        /*
         * ABERTURA EM MODO APPEND:
         * Adiciona ao final do arquivo sem sobrescrever
         */
        std::ofstream arquivo(nomeArquivo, std::ios::app);
        if (!arquivo.is_open()) {
            std::cerr << "Erro ao abrir arquivo de log: " << nomeArquivo << std::endl;
            return;
        }
        
        /*
         * ESCRITA DO CABEÇALHO:
         * Apenas se o arquivo não existe
         */
        if (!arquivoExiste) {
            arquivo << "NumThreads,OperacoesPorThread,TotalOperacoes,TempoExecucao_ms,"
                   << "OperacoesSucesso,OperacoesFalhas,TaxaSucesso,Throughput_ops_ms,"
                   << "Timestamp\n";
        }
        
        /*
         * CÁLCULO DE MÉTRICAS DERIVADAS:
         */
        int totalOperacoes = numThreads * operacoesPorThread;
        double taxaSucesso = (totalOperacoes > 0) ? 
                           (static_cast<double>(operacoesSucesso) / totalOperacoes) * 100 : 0;
        double throughput = (tempoExecucao > 0) ? 
                          static_cast<double>(operacoesSucesso) / tempoExecucao : 0;
        
        /*
         * GERAÇÃO DE TIMESTAMP:
         * Marca temporal para cada simulação
         */
        auto agora = std::chrono::system_clock::now();
        auto tempo_t = std::chrono::system_clock::to_time_t(agora);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&tempo_t), "%Y-%m-%d %H:%M:%S");
        
        /*
         * ESCRITA DOS DADOS:
         * Formato CSV com vírgula como separador
         */
        arquivo << numThreads << ","
                << operacoesPorThread << ","
                << totalOperacoes << ","
                << tempoExecucao << ","
                << operacoesSucesso << ","
                << operacoesFalhas << ","
                << std::fixed << std::setprecision(2) << taxaSucesso << ","
                << std::fixed << std::setprecision(4) << throughput << ","
                << ss.str() << "\n";
        
        arquivo.close();
        
        std::cout << "Log registrado: " << numThreads << " threads, " 
                  << tempoExecucao << "ms, " << operacoesSucesso << " sucessos, "
                  << operacoesFalhas << " falhas" << std::endl;
    }
    
    /*
     * LIMPEZA DE LOGS:
     * Remove o conteúdo do arquivo de log
     */
    void limparLogs() {
        std::lock_guard<std::mutex> lock(logMutex);
        std::ofstream arquivo(nomeArquivo, std::ios::trunc);
        arquivo.close();
        headerEscrito = false;
        std::cout << "Logs limpos do arquivo: " << nomeArquivo << std::endl;
    }
};

// ===================================
// Classe principal Sistema Bancário
// ===================================
class SistemaBancario {
private:
    std::unique_ptr<Banco> banco;                    // Instância do banco
    std::unique_ptr<SimuladorOperacoes> simulador;   // Simulador de operações
    std::unique_ptr<LoggerSimulacao> logger;         // Logger para CSV

public:
    /*
     * CONSTRUTOR:
     * Inicializa apenas o logger (banco será criado na inicialização)
     */
    SistemaBancario() : logger(std::make_unique<LoggerSimulacao>("simulacao_logs.csv")) {}

    /*
     * INICIALIZAÇÃO DO SISTEMA:
     * Cria instâncias e carrega dados iniciais
     */
    void inicializar() {
        std::cout << "=== INICIALIZANDO SISTEMA BANCÁRIO ===" << std::endl;
        
        /*
         * CRIAÇÃO DE INSTÂNCIAS:
         * make_unique cria objetos e retorna unique_ptr
         */
        banco = std::make_unique<Banco>();
        simulador = std::make_unique<SimuladorOperacoes>(*banco);
        
        /*
         * CARREGAMENTO DE DADOS:
         * Lê contas do arquivo texto
         */
        banco->carregarContas("ContaCorrente.txt");
    }

    /*
     * EXECUÇÃO DE SIMULAÇÃO ÚNICA:
     * Executa uma simulação com parâmetros específicos
     */
    void executarSimulacao(int numThreads, int operacoesPorThread) {
        std::cout << "\n=== INICIANDO SIMULAÇÃO ===" << std::endl;
        std::cout << "Threads: " << numThreads << std::endl;
        std::cout << "Operações por thread: " << operacoesPorThread << std::endl;

        /*
         * RESET DE ESTATÍSTICAS:
         * Limpa contadores antes de começar
         */
        banco->resetarEstatisticas();

        /*
         * MEDIÇÃO DE TEMPO:
         * Marca o início da simulação
         */
        auto inicio = std::chrono::high_resolution_clock::now();

        /*
         * CRIAÇÃO E EXECUÇÃO DE THREADS:
         * Cada thread executa o número especificado de operações
         */
        std::vector<std::thread> threads;

        // Cria threads
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back(&SimuladorOperacoes::executarOperacoes,
                                 simulador.get(), i, operacoesPorThread);
        }

        /*
         * SINCRONIZAÇÃO:
         * Espera todas as threads terminarem
         */
        for (auto& t : threads) {
            t.join();
        }

        /*
         * MEDIÇÃO FINAL:
         * Calcula tempo total de execução
         */
        auto fim = std::chrono::high_resolution_clock::now();
        auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(fim - inicio);

        /*
         * COLETA DE RESULTADOS:
         * Obtém estatísticas finais
         */
        int operacoesSucesso = banco->getOperacoesRealizadas();
        int operacoesFalhas = banco->getOperacoesFalhas();

        std::cout << "\n=== SIMULAÇÃO CONCLUÍDA ===" << std::endl;
        std::cout << "Tempo de execução: " << duracao.count() << " ms" << std::endl;

        /*
         * REGISTRO DE LOG:
         * Salva resultados no arquivo CSV
         */
        logger->registrarLogSimulacao(numThreads, operacoesPorThread, 
                                     duracao.count(), operacoesSucesso, 
                                     operacoesFalhas);

        /*
         * RELATÓRIOS E PERSISTÊNCIA:
         * Mostra estatísticas e salva estado final
         */
        banco->imprimirEstatisticas();
        banco->salvarContas("ContaCorrente.txt");
    }

    /*
     * MÚLTIPLAS SIMULAÇÕES:
     * Executa simulações com diferentes números de threads
     */
    void executarMultiplasSimulacoes() {
        /*
         * CONFIGURAÇÃO DE TESTE:
         * Array com diferentes números de threads para testar
         */
        std::vector<int> numThreads = {2, 4, 7, 12, 13};
        int operacoesPorThread = 50;

        std::cout << "=== INICIANDO MÚLTIPLAS SIMULAÇÕES ===" << std::endl;
        std::cout << "Será gerado arquivo: simulacao_logs.csv" << std::endl;

        /*
         * LOOP DE SIMULAÇÕES:
         * Para cada configuração de threads
         */
        for (int threads : numThreads) {
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "SIMULAÇÃO COM " << threads << " THREADS\n";
            std::cout << std::string(50, '=') << "\n";

            /*
             * RECREAÇÃO DE OBJETOS:
             * Garante estado limpo para cada simulação
             */
            banco = std::make_unique<Banco>();
            banco->carregarContas("ContaCorrente.txt");
            simulador = std::make_unique<SimuladorOperacoes>(*banco);

            /*
             * EXECUÇÃO DA SIMULAÇÃO:
             */
            executarSimulacao(threads, operacoesPorThread);
            
            /*
             * PAUSA ENTRE SIMULAÇÕES:
             * Evita sobrecarga do sistema
             */
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    /*
     * SIMULAÇÃO PERSONALIZADA:
     * Permite especificar configurações customizadas
     */
    void executarSimulacaoPersonalizada(const std::vector<int>& threads, 
                                       int operacoesPorThread) {
        std::cout << "=== SIMULAÇÃO PERSONALIZADA ===" << std::endl;
        
        for (int numThreads : threads) {
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "SIMULAÇÃO COM " << numThreads << " THREADS\n";
            std::cout << std::string(50, '=') << "\n";

            // Mesmo padrão: recria objetos para estado limpo
            banco = std::make_unique<Banco>();
            banco->carregarContas("ContaCorrente.txt");
            simulador = std::make_unique<SimuladorOperacoes>(*banco);

            executarSimulacao(numThreads, operacoesPorThread);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    /*
     * LIMPEZA DE LOGS:
     * Remove logs anteriores
     */
    void limparLogs() {
        logger->limparLogs();
    }
};

// ================================================================================================
// FUNÇÃO PRINCIPAL
// ================================================================================================
/*
 * Ponto de entrada do programa.
 * Coordena a execução completa do sistema.
 */
int main() {
    try {
        /*
         * INICIALIZAÇÃO:
         * Cria e inicializa o sistema bancário
         */
        SistemaBancario sistema;
        sistema.inicializar();
        std::cout << "Sistema bancario inicializado com sucesso" << std::endl;
        
        /*
         * LIMPEZA DE LOGS ANTERIORES:
         * Remove dados de execuções anteriores
         */
        sistema.limparLogs();
        
        /*
         * EXECUÇÃO DAS SIMULAÇÕES:
         * Roda simulações com diferentes números de threads
         * Os resultados são salvos automaticamente em CSV
         */
        sistema.executarMultiplasSimulacoes();
        
        /*
         * FINALIZAÇÃO:
         * Informa que o processo foi concluído
         */
        std::cout << "\n=== SISTEMA FINALIZADO ===" << std::endl;
        std::cout << "O arquivo 'simulacao_logs.csv' pronto." << std::endl;
        
    } catch (const std::exception& e) {
        /*
         * TRATAMENTO DE ERRO:
         * Captura exceções e exibe mensagem de erro
         */
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;  // Código de erro
    }
    
    return 0;
}