#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//estrutura do registro
struct Registro {
    int         id;
    std::string nome;
};

//banco de dados em memória
extern std::vector<Registro> tabela;
extern std::mutex            db_mutex;

//fila de requisições (produtor-consumidor)
extern std::queue<std::string>    fila_requisicoes;
extern std::mutex                 fila_mutex;
extern std::condition_variable    fila_cv;
extern bool                       encerrar;

//funções CRUD
void executar_insert (int id, const std::string& nome);
void executar_select (int id);         
void executar_delete (int id);
void executar_update (int id, const std::string& novo_nome);

//parser de requisição
void processar_requisicao(const std::string& req);

//worker das threads
void worker(int thread_id);

const char* PIPE_NAME = "/tmp/db_pipe";