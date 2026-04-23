//para compilar:  g++ -std=c++17 servidor.cpp -o servidor -lpthread
//para executar:  ./servidor

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "banco.h"

//definições das variáveis globais declaradas em banco.h
std::vector<Registro>   tabela;
std::mutex              db_mutex;
std::queue<std::string> fila_requisicoes;
std::mutex              fila_mutex;
std::condition_variable fila_cv;
bool                    encerrar = false;

//utilitário
static std::mutex log_mutex;

void log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(log_mutex);
    std::cout << msg << "\n";

    std::ofstream f("banco.log", std::ios::app);
    if (f.is_open()) f << msg << "\n";
}

//persistência: salva tabela inteira em banco.txt
//(chamada sempre que há INSERT, DELETE ou UPDATE)
static void salvar_tabela() {
    //db_mutex já deve estar travado pelo chamador
    std::ofstream f("banco.txt");
    if (!f.is_open()) return;
    f << "id,nome\n";
    for (const auto& r : tabela)
        f << r.id << "," << r.nome << "\n";
}

//carga inicial: lê banco.txt se existir
static void carregar_tabela() {
    std::ifstream f("banco.txt");
    if (!f.is_open()) return;

    std::string linha;
    std::getline(f, linha); //pula cabeçalho "id,nome"

    while (std::getline(f, linha)) {
        if (linha.empty()) continue;
        std::istringstream ss(linha);
        std::string sid, snome;
        std::getline(ss, sid,   ',');
        std::getline(ss, snome, ',');
        if (!sid.empty() && !snome.empty())
            tabela.push_back({std::stoi(sid), snome});
    }
    log("[SERVIDOR] Tabela carregada: " + std::to_string(tabela.size()) + " registro(s).");
}

//CRUD
void executar_insert(int id, const std::string& nome) {
    std::lock_guard<std::mutex> lk(db_mutex);

    //verifica duplicata
    for (const auto& r : tabela) {
        if (r.id == id) {
            log("[INSERT] ERRO: id=" + std::to_string(id) + " ja existe.");
            return;
        }
    }

    tabela.push_back({id, nome});
    salvar_tabela();
    log("[INSERT] id=" + std::to_string(id) + " nome=" + nome);
}

void executar_select(int id) {
    std::lock_guard<std::mutex> lk(db_mutex);

    if (id == -1) {
        log("[SELECT] Todos os registros (" + std::to_string(tabela.size()) + "):");
        for (const auto& r : tabela)
            log("  -> id=" + std::to_string(r.id) + " nome=" + r.nome);
    } else {
        bool found = false;
        for (const auto& r : tabela) {
            if (r.id == id) {
                log("[SELECT] id=" + std::to_string(r.id) + " nome=" + r.nome);
                found = true;
                break;
            }
        }
        if (!found)
            log("[SELECT] id=" + std::to_string(id) + " nao encontrado.");
    }
}

void executar_delete(int id) {
    std::lock_guard<std::mutex> lk(db_mutex);

    auto it = std::remove_if(tabela.begin(), tabela.end(),
        [id](const Registro& r){ return r.id == id; });

    if (it == tabela.end()) {
        log("[DELETE] ERRO: id=" + std::to_string(id) + " nao encontrado.");
        return;
    }
    tabela.erase(it, tabela.end());
    salvar_tabela();
    log("[DELETE] id=" + std::to_string(id) + " removido.");
}

void executar_update(int id, const std::string& novo_nome) {
    std::lock_guard<std::mutex> lk(db_mutex);

    for (auto& r : tabela) {
        if (r.id == id) {
            log("[UPDATE] id=" + std::to_string(id) +
                " nome: " + r.nome + " -> " + novo_nome);
            r.nome = novo_nome;
            salvar_tabela();
            return;
        }
    }
    log("[UPDATE] ERRO: id=" + std::to_string(id) + " nao encontrado.");
}

//parser de requisição simples
void processar_requisicao(const std::string& req) {
    std::istringstream ss(req);
    std::string op;
    ss >> op;

    //converte para maiúsculo
    for (auto& c : op) c = std::toupper(c);

    if (op == "INSERT") {
        std::string sid, snome;
        ss >> sid >> snome;                 
        int id = std::stoi(sid.substr(3)); 
        std::string nome = snome.substr(5);
        executar_insert(id, nome);

    } else if (op == "SELECT") {
        std::string token;
        ss >> token;
        if (token == "*") {
            executar_select(-1);
        } else {
            int id = std::stoi(token.substr(3));
            executar_select(id);
        }

    } else if (op == "DELETE") {
        std::string sid;
        ss >> sid;
        int id = std::stoi(sid.substr(3));
        executar_delete(id);

    } else if (op == "UPDATE") {
        std::string sid, snome;
        ss >> sid >> snome;
        int id = std::stoi(sid.substr(3));
        std::string novo_nome = snome.substr(5);
        executar_update(id, novo_nome);

    } else {
        log("[PARSER] Operacao desconhecida: " + op);
    }
}

//worker: cada thread do pool executa esta função
//padrão produtor-consumidor com condition_variable
void worker(int thread_id) {
    log("[THREAD " + std::to_string(thread_id) + "] Iniciada.");

    while (true) {
        std::unique_lock<std::mutex> lk(fila_mutex);

        //bloqueia até ter trabalho OU sinal de encerramento
        fila_cv.wait(lk, []{
            return !fila_requisicoes.empty() || encerrar;
        });

        //encerra se não há mais trabalho
        if (encerrar && fila_requisicoes.empty()) break;

        //pega a próxima requisição
        std::string req = fila_requisicoes.front();
        fila_requisicoes.pop();
        lk.unlock(); //libera o mutex da fila antes de processar

        log("[THREAD " + std::to_string(thread_id) + "] Processando: " + req);
        processar_requisicao(req);
    }

    log("[THREAD " + std::to_string(thread_id) + "] Encerrada.");
}

//main: cria o pool de threads e enfileira as requisições
int main() {
    const int NUM_THREADS = 4;
    log("=== Servidor BD Simulado ===");
    
    carregar_tabela();

    // Cria o pipe (FIFO) se não existir
    mkfifo(PIPE_NAME, 0666);

    std::vector<std::thread> pool;
    for (int i = 0; i < NUM_THREADS; i++)
        pool.emplace_back(worker, i + 1);

    // Loop de escuta do IPC
    log("[SERVIDOR] Aguardando conexões via Pipe: " + std::string(PIPE_NAME));
    
    while (!encerrar) {
        // Abre o pipe para leitura
        int fd = open(PIPE_NAME, O_RDONLY);
        if (fd != -1) {
            char buffer[256];
            ssize_t bytesRead;
            
            // Lê comandos enviados pelo cliente
            while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
                std::string comando(buffer);
                
                if (comando == "SAIR_SERVER") {
                    encerrar = true;
                    break;
                }

                // Enfileira a requisição para as threads
                {
                    std::lock_guard<std::mutex> lk(fila_mutex);
                    fila_requisicoes.push(comando);
                }
                fila_cv.notify_one();
            }
            close(fd);
        }
    }

    // Notifica threads para encerramento
    fila_cv.notify_all();
    for (auto& t : pool) t.join();

    unlink(PIPE_NAME); // Remove o pipe ao fechar
    log("=== Servidor encerrado ===");
    return 0;
}