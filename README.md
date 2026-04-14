# parallel-db-server

Simulação de um gerenciador de banco de dados com processamento paralelo de requisições, desenvolvido para a disciplina de **Sistemas Operacionais** da Universidade do Vale do Itajaí (UNIVALI).

O sistema demonstra na prática os conceitos de **IPC**, **threads**, **mutex** e **concorrência**, simulando o funcionamento interno de um SGBD leve com suporte a operações SQL básicas.

Esta versão corresponde à **entrega parcial da M1**, cujo foco é o **servidor com pool de threads funcionando**. O cliente com IPC real será concluído na etapa final do trabalho.

## Informações acadêmicas

- **Disciplina:** Sistemas Operacionais
- **Professor:** Felipe Viel
- **Instituição:** Universidade do Vale do Itajaí (UNIVALI)
- **Dupla:** Maria Eduarda Lima da Silva e Nicoly Maria Tejero Colutti

## Visão geral

```text
Cliente (futuro IPC)
        |
        v pipe / memória compartilhada
+---------------------------------------+
|              Servidor                 |
|                                       |
|  Fila de requisições (mutex)          |
|        |                              |
|   +----v----+  +---------+            |
|   |Thread 1 |  |Thread 2 |  ...       |
|   +----+----+  +----+----+            |
|        +-----+-----+                  |
|           db_mutex                    |
|               |                       |
|         Tabela (vetor)                |
|               |                       |
|           banco.txt                   |
+---------------------------------------+
```

## Funcionalidades

| Operação | Exemplo de requisição      | Descrição                        |
|----------|----------------------------|----------------------------------|
| INSERT   | `INSERT id=7 nome=Joao`    | Adiciona um registro             |
| SELECT   | `SELECT id=5` / `SELECT *` | Busca por ID ou lista todos      |
| DELETE   | `DELETE id=3`              | Remove um registro por ID        |
| UPDATE   | `UPDATE id=2 nome=Carlos`  | Atualiza o nome de um registro   |

## Arquitetura

### Componentes

- **`banco.h`** - structs, variáveis globais compartilhadas e declarações de funções
- **`servidor.cpp`** - servidor principal: pool de threads, fila de requisições, CRUD e persistência
- **`cliente.cpp`** - placeholder para comunicação IPC via pipe ou memória compartilhada
- **`banco.txt`** - persistência dos dados entre execuções
- **`banco.log`** - log das operações executadas

### Conceitos implementados

**Pool de threads** - 4 threads são criadas na inicialização e reutilizadas durante toda a execução, evitando o custo de criar e destruir threads a cada requisição.

**Padrão produtor-consumidor** - o processo principal enfileira requisições em `std::queue` e as threads as consomem. A sincronização usa `std::condition_variable`: as threads ficam bloqueadas enquanto a fila está vazia e são acordadas com `notify_all()`.

**Dois mutexes independentes**

- `fila_mutex` - protege a fila de requisições
- `db_mutex` - protege o vetor `tabela`, que representa o banco em memória

Separar os mutexes permite que uma thread processe uma operação no banco sem impedir outra thread de retirar a próxima requisição da fila.

**`lock_guard` e `unique_lock`** - o código usa `lock_guard` nas operações CRUD, onde o escopo do lock e fixo, e `unique_lock` no worker, pois a `condition_variable` precisa liberar e readquirir o mutex durante o `wait`.

## Estado atual da entrega parcial

Nesta fase, as requisições de teste são enfileiradas internamente no próprio `servidor.cpp` para demonstrar o funcionamento das threads, da fila compartilhada e da sincronização.

Ou seja, **ainda não há IPC real implementado nesta entrega parcial**. O `cliente.cpp` existe como placeholder e será concluído na entrega final, conforme a proposta do trabalho.

## Como compilar e executar

### Servidor

Com `g++`:

```bash
g++ -std=c++17 servidor.cpp -o servidor -lpthread
./servidor
```

No Windows, após compilar:

```powershell
.\servidor.exe
```

### Cliente

O cliente atual e apenas ilustrativo:

```bash
g++ -std=c++17 cliente.cpp -o cliente
./cliente
```

## Saída esperada

```text
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
