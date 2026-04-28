# parallel-db-server

Simulação de um gerenciador de banco de dados com processamento paralelo de requisições, desenvolvido para a disciplina de **Sistemas Operacionais** da Universidade do Vale do Itajaí (UNIVALI).

O sistema demonstra na prática os conceitos de **IPC**, **threads**, **mutex** e **concorrência**, simulando o funcionamento interno de um SGBD leve com suporte a operações SQL básicas.



---

## Informações acadêmicas

| | |
|---|---|
| **Disciplina** | Sistemas Operacionais |
| **Professor** | Felipe Viel |
| **Instituição** | Universidade do Vale do Itajaí (UNIVALI) |
| **Dupla** | Maria Eduarda Lima da Silva e Nicoly Maria Tejero Colutti |

---

## Visão geral

```
Cliente (futuro IPC)
        │
        ▼ pipe / memória compartilhada
┌───────────────────────────────────────┐
│              Servidor                 │
│                                       │
│  Fila de requisições (mutex)          │
│        │                              │
│   ┌────▼────┐  ┌─────────┐           │
│   │Thread 1 │  │Thread 2 │  ...       │
│   └────┬────┘  └────┬────┘           │
│        └──────┬─────┘                │
│           db_mutex                   │
│               │                      │
│         Tabela (vetor)               │
│               │                      │
│           banco.txt                  │
└───────────────────────────────────────┘
```

---

## Funcionalidades

| Operação | Exemplo de requisição         | Descrição                         |
|----------|-------------------------------|-----------------------------------|
| INSERT   | `INSERT id=7 nome=Joao`       | Adiciona um registro              |
| SELECT   | `SELECT id=5` / `SELECT *`    | Busca por ID ou lista todos       |
| DELETE   | `DELETE id=3`                 | Remove um registro por ID         |
| UPDATE   | `UPDATE id=2 nome=Carlos`     | Atualiza o nome de um registro    |

---

## Arquitetura

### Componentes

- **`banco.h`** — structs, variáveis globais compartilhadas e declarações de funções
- **`servidor.cpp`** — servidor principal: pool de threads, fila de requisições, CRUD e persistência
- **`cliente.cpp`** — cliente interativo via pipe (entrega final)
- **`banco.txt`** — persistência dos dados entre execuções
- **`banco.log`** — log das operações executadas

### Conceitos implementados

**Pool de threads** — 4 threads criadas na inicialização e reutilizadas durante toda a execução, evitando o custo de criar e destruir threads a cada requisição.

**Padrão produtor-consumidor** — o processo principal enfileira requisições (`std::queue`) e as threads as consomem. A sincronização usa `std::condition_variable`: as threads ficam bloqueadas enquanto a fila está vazia e são acordadas com `notify_one()`.

**Dois mutexes independentes:**
- `fila_mutex` — protege a fila de requisições
- `db_mutex` — protege o vetor `tabela` (banco em memória)

Separar os mutexes permite que uma thread processe uma operação no banco sem impedir outra de retirar a próxima requisição da fila.

**`lock_guard` vs `unique_lock`** — o código usa `lock_guard` nas operações CRUD, onde o escopo do lock é fixo, e `unique_lock` no worker, pois a `condition_variable` precisa liberar e readquirir o mutex durante o `wait`.

---

## Como compilar e executar

### Pré-requisitos

- GCC com suporte a C++17
- Linux, WSL ou macOS

### Servidor

```bash
g++ -std=c++17 servidor.cpp -o servidor -lpthread
./servidor
```

### Cliente

```bash
g++ -std=c++17 cliente.cpp -o cliente
./cliente
```

### Saída esperada

```
=== Servidor BD Simulado ===
Pool de threads: 4
[THREAD 1] Iniciada.
[THREAD 2] Iniciada.
[THREAD 2] Processando: INSERT id=2 nome=Nicoly
[THREAD 4] Iniciada.
...
[INSERT] id=2 nome=Nicoly
[SELECT] Todos os registros (4):
  -> id=1 nome=MariaEduarda
  -> id=2 nome=Nicoly
...
=== Servidor encerrado. Verifique banco.txt e banco.log ===
```

---

## Estrutura de arquivos

```
parallel-db-server/
├── banco.h        # Structs e declarações
├── servidor.cpp   # Servidor com pool de threads e IPC
├── cliente.cpp    # Cliente interativo via pipe
├── Makefile       # Atalho de compilação
├── banco.txt      # Banco de dados persistido
└── banco.log      # Log de operações
```

---


## Disciplina

**Sistemas Operacionais** — Escola Politécnica, UNIVALI  
Avaliação M1 — IPC, Threads e Paralelismo  
Professor: Felipe Viel
