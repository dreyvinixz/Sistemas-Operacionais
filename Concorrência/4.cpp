#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <shared_mutex>
#include <vector>
#include <map>
#include <random>
#include <chrono>
#include <string>
#include <sstream>
#include <atomic>
#include <iomanip>

// ===================================
// Classe ContaCorrente
// ===================================
class ContaCorrente {
private:
    std::string identificador;
    double saldo;
    mutable std::shared_mutex rwMutex;
    mutable std::atomic<int> leitoresAtivos{0};

public:
    ContaCorrente(const std::string& id, double saldoInicial) 
        : identificador(id), saldo(saldoInicial) {}

    std::string getId() const { return identificador; }

    bool creditar(double valor) {
        std::unique_lock<std::shared_mutex> lock(rwMutex);
        if (valor <= 0) return false;
        saldo += valor;
        std::cout << "[CRÉDITO] Conta " << identificador 
                  << " - Valor: R$ " << std::fixed << std::setprecision(2) << valor
                  << " - Novo saldo: R$ " << saldo << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return true;
    }

    bool debitar(double valor) {
        std::unique_lock<std::shared_mutex> lock(rwMutex);
        if (valor <= 0 || saldo < valor) return false;
        saldo -= valor;
        std::cout << "[DÉBITO] Conta " << identificador 
                  << " - Valor: R$ " << std::fixed << std::setprecision(2) << valor
                  << " - Novo saldo: R$ " << saldo << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return true;
    }

    double consultarSaldo() const {
        std::shared_lock<std::shared_mutex> lock(rwMutex);
        if (leitoresAtivos.load() >= 5) return -1;
        leitoresAtivos++;
        std::cout << "[CONSULTA] Conta " << identificador 
                  << " - Saldo: R$ " << std::fixed << std::setprecision(2) << saldo
                  << " - Leitores ativos: " << leitoresAtivos.load() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        double saldoAtual = saldo;
        leitoresAtivos--;
        return saldoAtual;
    }

    double getSaldoUnsafe() const {
        return saldo;
    }
};

// ===================================
// Classe Banco
// ===================================
class Banco {
private:
    std::map<std::string, std::unique_ptr<ContaCorrente>> contas;
    std::mutex contasMutex;
    std::atomic<int> operacoesRealizadas{0};
    std::atomic<int> operacoesFalhas{0};

public:
    void carregarContas(const std::string& arquivo) {
        std::ifstream file(arquivo);
        if (!file.is_open()) {
            throw std::runtime_error("Arquivo ContaCorrente.txt não encontrado.");
        }

        std::string linha;
        while (std::getline(file, linha)) {
            std::stringstream ss(linha);
            std::string id;
            double saldo;
            if (std::getline(ss, id, '|') && ss >> saldo) {
                std::lock_guard<std::mutex> lock(contasMutex);
                contas[id] = std::make_unique<ContaCorrente>(id, saldo);
            }
        }

        std::cout << "Carregadas " << contas.size() << " contas do arquivo." << std::endl;
    }

    void salvarContas(const std::string& arquivo) {
        std::ofstream file(arquivo);
        if (!file.is_open()) {
            std::cerr << "Erro ao salvar no arquivo: " << arquivo << std::endl;
            return;
        }

        std::lock_guard<std::mutex> lock(contasMutex);
        for (const auto& [id, conta] : contas) {
            file << id << "|" << std::fixed << std::setprecision(2) 
                 << conta->getSaldoUnsafe() << std::endl;
        }

        std::cout << "Contas salvas no arquivo: " << arquivo << std::endl;
    }

    ContaCorrente* obterConta(const std::string& id) {
        std::lock_guard<std::mutex> lock(contasMutex);
        auto it = contas.find(id);
        return (it != contas.end()) ? it->second.get() : nullptr;
    }

    std::vector<std::string> listarContas() {
        std::lock_guard<std::mutex> lock(contasMutex);
        std::vector<std::string> ids;
        for (const auto& [id, conta] : contas) {
            ids.push_back(id);
        }
        return ids;
    }

    void incrementarOperacoes() { operacoesRealizadas++; }
    void incrementarFalhas() { operacoesFalhas++; }

    int getOperacoesRealizadas() const { return operacoesRealizadas.load(); }
    int getOperacoesFalhas() const { return operacoesFalhas.load(); }

    void resetarEstatisticas() {
        operacoesRealizadas.store(0);
        operacoesFalhas.store(0);
    }

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
    Banco& banco;
    std::mt19937 rng;
    std::uniform_int_distribution<> operacaoDist;
    std::uniform_real_distribution<> valorDist;
    std::atomic<bool> executando{true};

public:
    SimuladorOperacoes(Banco& b) : banco(b), rng(std::random_device{}()), 
                                   operacaoDist(0, 2), valorDist(10.0, 500.0) {}

    void executarOperacoes(int threadId, int numOperacoes) {
        auto contas = banco.listarContas();
        if (contas.empty()) return;

        std::uniform_int_distribution<> contaDist(0, contas.size() - 1);

        for (int i = 0; i < numOperacoes && executando; ++i) {
            std::string contaId = contas[contaDist(rng)];
            ContaCorrente* conta = banco.obterConta(contaId);
            if (!conta) {
                banco.incrementarFalhas();
                continue;
            }

            int operacao = operacaoDist(rng);
            double valor = valorDist(rng);
            bool sucesso = false;

            switch (operacao) {
                case 0: sucesso = conta->creditar(valor); break;
                case 1: sucesso = conta->debitar(valor); break;
                case 2: sucesso = (conta->consultarSaldo() >= 0); break;
            }

            sucesso ? banco.incrementarOperacoes() : banco.incrementarFalhas();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        std::cout << "Thread " << threadId << " finalizou suas operações.\n";
    }

    void pararExecucao() {
        executando.store(false);
    }
};

// ===================================
// Classe Logger para Simulações
// ===================================
class LoggerSimulacao {
private:
    std::mutex logMutex;
    std::string nomeArquivo;
    bool headerEscrito;

public:
    LoggerSimulacao(const std::string& arquivo) : nomeArquivo(arquivo), headerEscrito(false) {}

    void registrarLogSimulacao(int numThreads, int operacoesPorThread, 
                              long long tempoExecucao, int operacoesSucesso, 
                              int operacoesFalhas) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        // Verifica se arquivo existe para decidir se escreve header
        std::ifstream teste(nomeArquivo);
        bool arquivoExiste = teste.good();
        teste.close();
        
        std::ofstream arquivo(nomeArquivo, std::ios::app);
        if (!arquivo.is_open()) {
            std::cerr << "Erro ao abrir arquivo de log: " << nomeArquivo << std::endl;
            return;
        }
        
        // Escreve header apenas se arquivo não existe
        if (!arquivoExiste) {
            arquivo << "NumThreads,OperacoesPorThread,TotalOperacoes,TempoExecucao_ms,"
                   << "OperacoesSucesso,OperacoesFalhas,TaxaSucesso,Throughput_ops_ms,"
                   << "Timestamp\n";
        }
        
        // Calcula métricas derivadas
        int totalOperacoes = numThreads * operacoesPorThread;
        double taxaSucesso = (totalOperacoes > 0) ? 
                           (static_cast<double>(operacoesSucesso) / totalOperacoes) * 100 : 0;
        double throughput = (tempoExecucao > 0) ? 
                          static_cast<double>(operacoesSucesso) / tempoExecucao : 0;
        
        // Obtém timestamp atual
        auto agora = std::chrono::system_clock::now();
        auto tempo_t = std::chrono::system_clock::to_time_t(agora);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&tempo_t), "%Y-%m-%d %H:%M:%S");
        
        // Escreve dados da simulação
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
    std::unique_ptr<Banco> banco;
    std::unique_ptr<SimuladorOperacoes> simulador;
    std::unique_ptr<LoggerSimulacao> logger;

public:
    SistemaBancario() : logger(std::make_unique<LoggerSimulacao>("simulacao_logs.csv")) {}

    void inicializar() {
        std::cout << "=== INICIALIZANDO SISTEMA BANCÁRIO ===" << std::endl;
        banco = std::make_unique<Banco>();
        simulador = std::make_unique<SimuladorOperacoes>(*banco);
        banco->carregarContas("ContaCorrente.txt");
    }

    void executarSimulacao(int numThreads, int operacoesPorThread) {
        std::cout << "\n=== INICIANDO SIMULAÇÃO ===" << std::endl;
        std::cout << "Threads: " << numThreads << std::endl;
        std::cout << "Operações por thread: " << operacoesPorThread << std::endl;

        // Reset estatísticas antes de começar
        banco->resetarEstatisticas();

        auto inicio = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;

        // Cria e inicia threads
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back(&SimuladorOperacoes::executarOperacoes,
                                 simulador.get(), i, operacoesPorThread);
        }

        for (auto& t : threads) {
            t.join();
        }

        auto fim = std::chrono::high_resolution_clock::now();
        auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(fim - inicio);

        // Coleta estatísticas finais
        int operacoesSucesso = banco->getOperacoesRealizadas();
        int operacoesFalhas = banco->getOperacoesFalhas();

        std::cout << "\n=== SIMULAÇÃO CONCLUÍDA ===" << std::endl;
        std::cout << "Tempo de execução: " << duracao.count() << " ms" << std::endl;

        // Registra log da simulação
        logger->registrarLogSimulacao(numThreads, operacoesPorThread, 
                                     duracao.count(), operacoesSucesso, 
                                     operacoesFalhas);

        banco->imprimirEstatisticas();
        banco->salvarContas("ContaCorrente.txt");
    }

    void executarMultiplasSimulacoes() {
        std::vector<int> numThreads = {2, 4, 7, 12, 13};
        int operacoesPorThread = 50;

        std::cout << "=== INICIANDO MÚLTIPLAS SIMULAÇÕES ===" << std::endl;
        std::cout << "Será gerado arquivo: simulacao_logs.csv" << std::endl;

        for (int threads : numThreads) {
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "SIMULAÇÃO COM " << threads << " THREADS\n";
            std::cout << std::string(50, '=') << "\n";

            // Recria banco e simulador do zero
            banco = std::make_unique<Banco>();
            banco->carregarContas("ContaCorrente.txt");
            simulador = std::make_unique<SimuladorOperacoes>(*banco);

            executarSimulacao(threads, operacoesPorThread);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void executarSimulacaoPersonalizada(const std::vector<int>& threads, 
                                       int operacoesPorThread) {
        std::cout << "=== SIMULAÇÃO PERSONALIZADA ===" << std::endl;
        
        for (int numThreads : threads) {
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "SIMULAÇÃO COM " << numThreads << " THREADS\n";
            std::cout << std::string(50, '=') << "\n";

            // Recria banco e simulador do zero
            banco = std::make_unique<Banco>();
            banco->carregarContas("ContaCorrente.txt");
            simulador = std::make_unique<SimuladorOperacoes>(*banco);

            executarSimulacao(numThreads, operacoesPorThread);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void limparLogs() {
        logger->limparLogs();
    }
};

// Função principal
int main() {
    try {
        SistemaBancario sistema;
        sistema.inicializar();
        std::cout << "Sistema bancario inicializado com sucesso" << std::endl;
        sistema.limparLogs();
        
        // Executa simulações com diferentes números de threads
        sistema.executarMultiplasSimulacoes();
        
        std::cout << "\n=== SISTEMA FINALIZADO ===" << std::endl;
        std::cout << "O arquivo 'simulacao_logs.csv' pronto." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}