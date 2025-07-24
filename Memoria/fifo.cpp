#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <map>

using namespace std;

struct Processo {
    int tempoCriacao;
    int pid;
    int tempoExecucao;
    int prioridade;
    int qtdeMemoria;
    vector<int> sequenciaAcesso;
    int numPaginas;  // Calculado com base na qtdeMemoria
};

struct ConfiguracaoSistema {
    string algoritmoEscalonamento;
    int fracaoCPU;
    string politicaMemoria;
    int tamanhoMemoria;
    int tamanhoPaginasMolduras;
    int percentualAlocacao;
    int totalMolduras;  // Calculado
};

struct AcessoPagina {
    int tempo;
    int pid;
    int pagina;
};

class SimuladorFIFO {
private:
    ConfiguracaoSistema config;
    vector<Processo> processos;
    
public:
    SimuladorFIFO(const ConfiguracaoSistema& cfg, const vector<Processo>& procs) 
        : config(cfg), processos(procs) {}
    
    int executarFIFO() {
        if (config.politicaMemoria == "local") {
            return executarPoliticaLocal();
        } else {
            return executarPoliticaGlobal();
        }
    }
    
private:
    int executarPoliticaLocal() {
        int totalTrocas = 0;
        
        for (const auto& processo : processos) {
            totalTrocas += simularProcessoLocal(processo);
        }
        
        return totalTrocas;
    }
    
    int executarPoliticaGlobal() {
        // Cria lista de todos os acessos ordenados por tempo
        vector<AcessoPagina> todosAcessos;
        gerarSequenciaAcessosGlobal(todosAcessos);
        
        // Simula FIFO global
        queue<pair<int, int>> filaFIFO;  // (pid, pagina)
        unordered_set<string> paginasNaMemoria;  // "pid:pagina"
        int trocas = 0;
        
        for (const auto& acesso : todosAcessos) {
            string chave = to_string(acesso.pid) + ":" + to_string(acesso.pagina);
            
            if (paginasNaMemoria.find(chave) != paginasNaMemoria.end()) {
                // Hit - página já está na memória
                continue;
            }
            
            // Miss - página não está na memória
            if (paginasNaMemoria.size() < config.totalMolduras) {
                // Ainda há espaço
                paginasNaMemoria.insert(chave);
                filaFIFO.push({acesso.pid, acesso.pagina});
            } else {
                // Precisa substituir
                auto removida = filaFIFO.front();
                filaFIFO.pop();
                string chaveRemovida = to_string(removida.first) + ":" + to_string(removida.second);
                paginasNaMemoria.erase(chaveRemovida);
                
                paginasNaMemoria.insert(chave);
                filaFIFO.push({acesso.pid, acesso.pagina});
                trocas++;
            }
        }
        
        return trocas;
    }
    void gerarSequenciaAcessosGlobal(vector<AcessoPagina>& todosAcessos) {
        for (const auto& processo : processos) {
            int tempoAtual = processo.tempoCriacao;
            int acessosRestantes = processo.tempoExecucao;
            
            for (int i = 0; i < processo.sequenciaAcesso.size() && acessosRestantes > 0; i++) {
                int pagina = processo.sequenciaAcesso[i];
                
                if (pagina >= 1 && pagina <= processo.numPaginas) {
                    todosAcessos.push_back({tempoAtual, processo.pid, pagina});
                }
                
                tempoAtual++;
                acessosRestantes--;
            }
        }
        
        sort(todosAcessos.begin(), todosAcessos.end(), 
             [](const AcessoPagina& a, const AcessoPagina& b) {
                 return a.tempo < b.tempo;
             });
    }
    void gerarAcessosSequencial(vector<AcessoPagina>& todosAcessos) {
        for (const auto& processo : processos) {
            int tempoAtual = processo.tempoCriacao;
            int acessosRestantes = processo.tempoExecucao;
            
            for (int i = 0; i < processo.sequenciaAcesso.size() && acessosRestantes > 0; i++) {
                int pagina = processo.sequenciaAcesso[i];
                
                if (pagina >= 1 && pagina <= processo.numPaginas) {
                    todosAcessos.push_back({tempoAtual, processo.pid, pagina});
                }
                
                tempoAtual++;
                acessosRestantes--;
            }
        }
        
        sort(todosAcessos.begin(), todosAcessos.end(), 
             [](const AcessoPagina& a, const AcessoPagina& b) {
                 return a.tempo < b.tempo;
             });
    }
    
    int simularProcessoLocal(const Processo& processo) {
        queue<int> filaFIFO;
        unordered_set<int> paginasNaMemoria;
        int trocas = 0;
        
        // Calcula molduras para este processo na política local
        int molduras = min((config.totalMolduras * config.percentualAlocacao) / 100, 
                          processo.numPaginas);
        
        int acessosRestantes = processo.tempoExecucao;
        
        for (int i = 0; i < processo.sequenciaAcesso.size() && acessosRestantes > 0; i++) {
            int pagina = processo.sequenciaAcesso[i];
            
            // Valida página
            if (pagina < 1 || pagina > processo.numPaginas) {
                acessosRestantes--;
                continue;
            }
            
            if (paginasNaMemoria.find(pagina) != paginasNaMemoria.end()) {
                // Hit
                acessosRestantes--;
                continue;
            }
            
            // Miss
            if (paginasNaMemoria.size() < molduras) {
                // Ainda há espaço
                paginasNaMemoria.insert(pagina);
                filaFIFO.push(pagina);
            } else {
                // Precisa substituir
                int paginaRemovida = filaFIFO.front();
                filaFIFO.pop();
                paginasNaMemoria.erase(paginaRemovida);
                
                paginasNaMemoria.insert(pagina);
                filaFIFO.push(pagina);
                trocas++;
            }
            
            acessosRestantes--;
        }
        
        return trocas;
    }
};

ConfiguracaoSistema parseConfiguracaoSistema(const string& linha) {
    ConfiguracaoSistema config;
    stringstream ss(linha);
    string item;
    
    getline(ss, config.algoritmoEscalonamento, '|');
    getline(ss, item, '|'); config.fracaoCPU = stoi(item);
    getline(ss, config.politicaMemoria, '|');
    getline(ss, item, '|'); config.tamanhoMemoria = stoi(item);
    getline(ss, item, '|'); config.tamanhoPaginasMolduras = stoi(item);
    getline(ss, item, '|'); config.percentualAlocacao = stoi(item);
    
    config.totalMolduras = config.tamanhoMemoria / config.tamanhoPaginasMolduras;
    
    return config;
}

Processo parseProcesso(const string& linha, int tamanhoPagina) {
    Processo processo;
    stringstream ss(linha);
    string item;
    
    getline(ss, item, '|'); processo.tempoCriacao = stoi(item);
    getline(ss, item, '|'); processo.pid = stoi(item);
    getline(ss, item, '|'); processo.tempoExecucao = stoi(item);
    getline(ss, item, '|'); processo.prioridade = stoi(item);
    getline(ss, item, '|'); processo.qtdeMemoria = stoi(item);
    
    // Calcula número de páginas
    processo.numPaginas = (processo.qtdeMemoria + tamanhoPagina - 1) / tamanhoPagina;
    
    if (getline(ss, item, '|')) {
        stringstream paginasStream(item);
        string pagina;
        while (paginasStream >> pagina) {
            processo.sequenciaAcesso.push_back(stoi(pagina));
        }
    }
    
    return processo;
}

int main() {
    string nomeArquivo;
    cout << "Digite o nome do arquivo de entrada: ";
    cin >> nomeArquivo;
    
    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << nomeArquivo << endl;
        return 1;
    }
    
    string linha;
    
    if (!getline(arquivo, linha)) {
        cerr << "Erro ao ler configuração do sistema" << endl;
        return 1;
    }

    ConfiguracaoSistema config = parseConfiguracaoSistema(linha);
    
    cout << "=== CONFIGURAÇÃO DO SISTEMA ===" << endl;
    cout << "Algoritmo de Escalonamento: " << config.algoritmoEscalonamento << endl;
    cout << "Fração de CPU: " << config.fracaoCPU << endl;
    cout << "Política de Memória: " << config.politicaMemoria << endl;
    cout << "Tamanho da Memória: " << config.tamanhoMemoria << " bytes" << endl;
    cout << "Tamanho das Páginas/Molduras: " << config.tamanhoPaginasMolduras << " bytes" << endl;
    cout << "Percentual de Alocação: " << config.percentualAlocacao << "%" << endl;
    cout << "Total de Molduras: " << config.tamanhoMemoria / config.tamanhoPaginasMolduras << endl << endl;
 

    
    vector<Processo> processos;
    while (getline(arquivo, linha)) {
        if (!linha.empty()) {
            processos.push_back(parseProcesso(linha, config.tamanhoPaginasMolduras));
        }
    }
    
    arquivo.close();
    
    SimuladorFIFO simulador(config, processos);
    int trocasFIFO = simulador.executarFIFO();
    cout << "=== RESULTADOS DA SIMULAÇÃO FIFO ===" << endl;
    cout << "Total de Trocas de Páginas: " << trocasFIFO << endl;
    cout << trocasFIFO << "|x|x|x|FIFO" << endl;
    
    return 0;
}