#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>

// Estrutura para representar uma página na memória
struct MemoryFrame {
    int page_id;
    int process_id;
    int load_time;
    bool occupied;
    
    MemoryFrame() : page_id(-1), process_id(-1), load_time(-1), occupied(false) {}
    
    void load_page(int pid, int proc_id, int time) {
        page_id = pid;
        process_id = proc_id;
        load_time = time;
        occupied = true;
    }
    
    void clear() {
        page_id = -1;
        process_id = -1;
        load_time = -1;
        occupied = false;
    }
};

// Classe para representar um processo
class Process {
public:
    int pid;
    int creation_time;
    int execution_time;
    int priority;
    int memory_needed;
    std::vector<int> page_sequence;
    int remaining_time;
    int current_page_index;
    int max_frames; // número máximo de frames que este processo pode usar
    
    Process(int p, int ct, int et, int pr, int mem, const std::vector<int>& seq)
        : pid(p), creation_time(ct), execution_time(et), priority(pr), 
          memory_needed(mem), page_sequence(seq), remaining_time(et), 
          current_page_index(0), max_frames(0) {}
    
    bool has_more_pages() const {
        return current_page_index < page_sequence.size();
    }
    
    int get_next_page() {
        if (has_more_pages()) {
            return page_sequence[current_page_index++];
        }
        return -1;
    }
    
    void reset_page_sequence() {
        current_page_index = 0;
    }
};

// Simulador FIFO
class FIFOSimulator {
private:
    std::string scheduling_algorithm;
    int cpu_slice;
    std::string memory_policy;  // local ou global
    int memory_size;
    int page_frame_size;
    int allocation_percentage;
    
    std::vector<Process> processes;
    std::vector<MemoryFrame> memory;
    int total_frames;
    int current_time;
    int page_replacements; // contador de substituições (trocas)
    
    // Para política local - mapear processo -> seus frames
    std::map<int, std::vector<int>> process_frames; // PID -> índices dos frames
    std::map<int, int> process_fifo_pointer; // PID -> ponteiro FIFO circular
    
    // Para política global
    int global_fifo_pointer;
    
public:
    FIFOSimulator() : current_time(0), page_replacements(0), global_fifo_pointer(0) {}
    
    void load_configuration(const std::string& filename);
    void run_simulation();
    void print_results();
    
private:
    bool is_page_in_memory(int page_id, int process_id);
    int find_page_frame(int page_id, int process_id);
    void allocate_frames_to_processes();
    int load_page_local_policy(int page_id, int process_id);
    int load_page_global_policy(int page_id, int process_id);
    Process* get_running_process();
    void schedule_processes();
};

void FIFOSimulator::load_configuration(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << filename << std::endl;
        return;
    }
    
    // Lê primeira linha com configurações
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        
        std::getline(ss, scheduling_algorithm, '|');
        std::getline(ss, token, '|'); cpu_slice = std::stoi(token);
        std::getline(ss, memory_policy, '|');
        std::getline(ss, token, '|'); memory_size = std::stoi(token);
        std::getline(ss, token, '|'); page_frame_size = std::stoi(token);
        std::getline(ss, token); allocation_percentage = std::stoi(token);
    }
    
    total_frames = memory_size / page_frame_size;
    memory.resize(total_frames);
    
    // Lê processos
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        
        int creation_time, pid, execution_time, priority, memory_needed;
        
        std::getline(ss, token, '|'); creation_time = std::stoi(token);
        std::getline(ss, token, '|'); pid = std::stoi(token);
        std::getline(ss, token, '|'); execution_time = std::stoi(token);
        std::getline(ss, token, '|'); priority = std::stoi(token);
        std::getline(ss, token, '|'); memory_needed = std::stoi(token);
        
        std::string sequence_str;
        std::getline(ss, sequence_str);
        
        std::vector<int> page_sequence;
        std::stringstream seq_ss(sequence_str);
        std::string page_str;
        while (seq_ss >> page_str) {
            page_sequence.push_back(std::stoi(page_str));
        }
        
        processes.emplace_back(pid, creation_time, execution_time, priority, memory_needed, page_sequence);
    }
    
    file.close();
    allocate_frames_to_processes();
}

void FIFOSimulator::allocate_frames_to_processes() {
    for (auto& process : processes) {
        // Calcula quantas páginas o processo precisa
        int process_pages = (process.memory_needed + page_frame_size - 1) / page_frame_size;
        
        // Calcula quantos frames pode usar baseado no percentual
        int max_frames_by_percentage = (process_pages * allocation_percentage) / 100;
        process.max_frames = std::max(1, max_frames_by_percentage); // mínimo 1 frame
        
        // Para política local, aloca frames específicos para cada processo
        if (memory_policy == "local") {
            process_frames[process.pid] = std::vector<int>();
            process_fifo_pointer[process.pid] = 0;
            
            // Reserva frames para este processo (simplificado - em ordem)
            int frames_allocated = 0;
            for (int i = 0; i < total_frames && frames_allocated < process.max_frames; i++) {
                process_frames[process.pid].push_back(i);
                frames_allocated++;
            }
        }
    }
}

bool FIFOSimulator::is_page_in_memory(int page_id, int process_id) {
    return find_page_frame(page_id, process_id) != -1;
}

int FIFOSimulator::find_page_frame(int page_id, int process_id) {
    if (memory_policy == "local") {
        // Busca apenas nos frames alocados para este processo
        if (process_frames.count(process_id)) {
            for (int frame_idx : process_frames[process_id]) {
                if (memory[frame_idx].occupied && 
                    memory[frame_idx].page_id == page_id && 
                    memory[frame_idx].process_id == process_id) {
                    return frame_idx;
                }
            }
        }
    } else {
        // Política global - busca em toda memória
        for (int i = 0; i < total_frames; i++) {
            if (memory[i].occupied && 
                memory[i].page_id == page_id && 
                memory[i].process_id == process_id) {
                return i;
            }
        }
    }
    return -1;
}

int FIFOSimulator::load_page_local_policy(int page_id, int process_id) {
    if (!process_frames.count(process_id)) return 0;
    
    auto& frames = process_frames[process_id];
    int& fifo_ptr = process_fifo_pointer[process_id];
    
    // Primeiro, tenta encontrar frame vazio
    for (int frame_idx : frames) {
        if (!memory[frame_idx].occupied) {
            memory[frame_idx].load_page(page_id, process_id, current_time);
            return 0; // Não é uma troca, apenas carregamento
        }
    }
    
    // Se não há frames vazios, substitui usando FIFO circular
    int frame_to_replace = frames[fifo_ptr];
    memory[frame_to_replace].load_page(page_id, process_id, current_time);
    fifo_ptr = (fifo_ptr + 1) % frames.size();
    
    return 1; // É uma substituição (troca)
}

int FIFOSimulator::load_page_global_policy(int page_id, int process_id) {
    // Primeiro, tenta encontrar frame vazio
    for (int i = 0; i < total_frames; i++) {
        if (!memory[i].occupied) {
            memory[i].load_page(page_id, process_id, current_time);
            return 0; // Não é uma troca, apenas carregamento
        }
    }
    
    // Se não há frames vazios, substitui usando FIFO circular global
    memory[global_fifo_pointer].load_page(page_id, process_id, current_time);
    global_fifo_pointer = (global_fifo_pointer + 1) % total_frames;
    
    return 1; // É uma substituição (troca)
}

Process* FIFOSimulator::get_running_process() {
    // Implementação simplificada - retorna o primeiro processo ativo no tempo atual
    for (auto& process : processes) {
        if (process.creation_time <= current_time && process.remaining_time > 0) {
            return &process;
        }
    }
    return nullptr;
}

void FIFOSimulator::run_simulation() {
    std::cout << "=== Simulação FIFO - Algoritmo de Substituição de Página ===" << std::endl;
    std::cout << "Política de Memória: " << memory_policy << std::endl;
    std::cout << "Memória Total: " << memory_size << " bytes (" << total_frames << " frames)" << std::endl;
    std::cout << "Tamanho da Página/Frame: " << page_frame_size << " bytes" << std::endl;
    std::cout << "Percentual de Alocação: " << allocation_percentage << "%" << std::endl << std::endl;
    
    // Reset das sequências de páginas
    for (auto& process : processes) {
        process.reset_page_sequence();
    }
    
    // Simula execução ciclo por ciclo
    bool simulation_running = true;
    
    while (simulation_running) {
        simulation_running = false;
        
        // Verifica se há processos ativos
        for (auto& process : processes) {
            if (process.creation_time <= current_time && process.remaining_time > 0) {
                simulation_running = true;
                
                // Simula fatia de CPU
                int cpu_cycles = std::min(cpu_slice, process.remaining_time);
                
                for (int cycle = 0; cycle < cpu_cycles; cycle++) {
                    // Em cada ciclo de CPU, o processo faz um acesso à memória
                    if (process.has_more_pages()) {
                        int page_to_access = process.get_next_page();
                        
                        std::cout << "Tempo " << current_time << ": Processo " << process.pid 
                                  << " acessa página " << page_to_access;
                        
                        // Verifica se página está na memória
                        if (!is_page_in_memory(page_to_access, process.pid)) {
                            std::cout << " -> PAGE FAULT! ";
                            
                            // Carrega página usando política apropriada
                            int was_replacement;
                            if (memory_policy == "local") {
                                was_replacement = load_page_local_policy(page_to_access, process.pid);
                            } else {
                                was_replacement = load_page_global_policy(page_to_access, process.pid);
                            }
                            
                            if (was_replacement) {
                                page_replacements++;
                                std::cout << "Substituição realizada (Total: " << page_replacements << ")";
                            } else {
                                std::cout << "Página carregada em frame vazio";
                            }
                        } else {
                            std::cout << " -> HIT!";
                        }
                        
                        std::cout << std::endl;
                    }
                    
                    current_time++;
                    process.remaining_time--;
                    
                    if (process.remaining_time == 0) {
                        std::cout << "Processo " << process.pid << " finalizado no tempo " 
                                  << current_time << std::endl;
                        break;
                    }
                }
            }
        }
    }
    
    std::cout << std::endl << "=== Simulação Finalizada ===" << std::endl;
}

void FIFOSimulator::print_results() {
    std::cout << "Número total de substituições de página (FIFO): " << page_replacements << std::endl;
    
    // Mostra estado final da memória
    std::cout << std::endl << "Estado final da memória:" << std::endl;
    for (int i = 0; i < total_frames; i++) {
        if (memory[i].occupied) {
            std::cout << "Frame " << i << ": Página " << memory[i].page_id 
                      << " (Processo " << memory[i].process_id << ")" << std::endl;
        } else {
            std::cout << "Frame " << i << ": Vazio" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_entrada>" << std::endl;
        return 1;
    }
    
    FIFOSimulator simulator;
    simulator.load_configuration(argv[1]);
    simulator.run_simulation();
    simulator.print_results();
    
    return 0;
}