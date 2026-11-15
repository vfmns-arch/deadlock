# Deadlock

Implementação em C do algoritmo do banqueiro para simular prevenção de deadlock em um sistema com múltiplos recursos.

O programa lê a disponibilidade inicial dos recursos pela linha de comando, as demandas máximas dos clientes em `customer.txt` e as operações de requisição/liberação em `commands.txt`. O resultado da simulação é gravado em `result.txt`.

## Como compilar

Em ambientes POSIX/Linux/macOS:

```bash
make
```

## Como executar

```bash
./banker 10 5 7
```

Exemplo de estrutura de `customer.txt`:

```text
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```

Exemplo de `commands.txt`:

```text
RQ 0 0 0 2
RQ 2 3 0 2
*
RL 0 0 1 1
```

- `RQ`: solicita recursos
- `RL`: libera recursos
- `*`: imprime o estado atual dos recursos e matrizes
