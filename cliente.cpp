#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include "banco.h"

int main() {
    int fd = open(PIPE_NAME, O_WRONLY);
    if (fd == -1) {
        std::cerr << "[CLIENTE] Erro: Servidor nao esta rodando ou pipe nao existe.\n";
        return 1;
    }

    std::string comando;
    std::cout << "Conectado ao Servidor. Digite os comandos (ex: INSERT id=10 nome=Teste):\n";
    std::cout << "Digite 'SAIR' para encerrar o cliente.\n";

    while (true) {
    std::cout << "> ";
    std::getline(std::cin, comando);
    
    if (comando == "SAIR") break;

    // Abre o pipe, escreve e fecha para garantir o flush do comando
    int fd = open(PIPE_NAME, O_WRONLY);
    if (fd != -1) {
        write(fd, comando.c_str(), comando.length() + 1);
        close(fd);
    } else {
        std::cerr << "[ERRO] Não foi possível enviar comando ao servidor.\n";
    }
}

    close(fd);
    return 0;
}