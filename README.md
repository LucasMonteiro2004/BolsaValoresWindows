# Bolsa de Valores Windows

Trabalho prático de Sistemas Operativos II — Licenciatura em Engenharia Informática  
Universidade de Coimbra  
Data de entrega: 18 de Maio de 2024

## Grupo

- 2022153271 - Lucas Monteiro
- 2021133861 - Afonso Silva

---

## Sumário

Este projeto tem como objetivo a implementação de uma Bolsa de Valores online simplificada, servindo como estudo prático para conceitos de sistemas distribuídos e comunicação entre processos. O sistema permite que usuários registrem empresas, comprem e vendam ações, e visualizem informações detalhadas sobre transações e participantes do mercado, com foco na aplicação dos conhecimentos adquiridos em aula.

---

## Índice

1. [Introdução](#introdução)
2. [Estruturas de Dados](#estruturas-de-dados)
3. [Comunicação Bolsa – Board / BoardGUI](#comunicação-bolsa--board--boardgui)
4. [Comunicação Bolsa – Clientes](#comunicação-bolsa--clientes)
5. [Justificativas de Implementações Próprias](#justificativas-de-implementações-próprias)
6. [Tabela dos Requisitos Implementados](#tabela-dos-requisitos-implementados)

---

## Introdução

O principal objetivo deste trabalho é criar um sistema que simula, de forma simplificada, o funcionamento de uma bolsa de valores. Os utilizadores podem comprar e vender ações de empresas, cujas informações e preços são geridos centralizadamente. O sistema contempla registro de novas empresas e visualização de informações sobre transações e participantes.

A arquitetura envolve múltiplos programas que interagem através de mecanismos de IPC (memória partilhada, pipes nomeados, mutexes, semáforos, eventos), possibilitando a aplicação prática de técnicas avançadas de sincronização e comunicação.

---

## Estruturas de Dados

O projeto define diversas estruturas fundamentais para o funcionamento do sistema:

- **Empresa**: Representa uma empresa listada na bolsa.
  - `nomeEmp`: Nome da empresa
  - `valorAcao`: Valor atual das ações
  - `numAcoes`: Número de ações disponíveis
  - `UltimaTransicao`: String da última transação

- **CarteiraAcoes**: Representa a carteira de ações de um cliente.
  - `emp`: Array de empresas
  - `numEmpresasCarteira`: Número de empresas na carteira

- **Cliente**: Representa um cliente da bolsa.
  - `nome`: Nome do cliente
  - `saldo`: Saldo disponível
  - `senha`: Senha de acesso
  - `ligado`: Estado de conexão
  - `continua`: Controle do processo
  - `hpipe`: Handle para comunicação
  - `carteira`: Carteira de ações do cliente
  - `cmd`: Comandos e dados para comunicação

- **TDATA (Bolsa)**: Estrutura de controle central da bolsa
  - Flags de execução
  - Handles para sincronização (mutex, eventos, semáforo)
  - Arrays de empresas e clientes

---

## Comunicação Bolsa – Board / BoardGUI

A comunicação entre os componentes "Bolsa" e "Board" é realizada via memória partilhada e sincronização de eventos:

- **Mutexes**: Garantem exclusividade de acesso à memória partilhada.
- **Eventos**: Sincronizam o ciclo de comunicação entre Bolsa e Board.
- **Memória Partilhada**: Utilizada para troca de dados sobre empresas entre os processos.

Fluxo resumido:
- Inicialização e ordenação do array de ações.
- Sincronização por evento.
- Mapeamento, leitura/escrita e desmapeamento da memória partilhada.
- Proteção do acesso por mutex.
- Atualização das informações de empresas conforme estado do sistema.

---

## Comunicação Bolsa – Clientes

A comunicação entre Bolsa (servidor) e Clientes é feita por pipes nomeados e operações de I/O sobrepostas:

- **Pipes Nomeados**: Canal de comunicação entre servidor e clientes.
- **I/O Overlapped**: Permite operações assíncronas de leitura e escrita.
- **Threads**: Cada cliente e o servidor operam em threads separadas para gerenciar comunicação independente.
- **Sincronização**: Uso de mutexes, semáforos e eventos para garantir integridade dos dados e controle de conexões.

Fluxo resumido:
- Inicialização de objetos de sincronização pelo servidor.
- Criação de pipes nomeados e espera por conexões.
- Comunicação baseada em comandos string (login, exit, listc, balance, buy, sell, listcarteira).
- Processamento de comandos e respostas pelo servidor, com atualização dos estados dos clientes.

---

## Justificativas de Implementações Próprias

- **Comandos Extras**:
  - `listarCarteira` (Cliente): Permite ao utilizador visualizar empresas e ações na sua carteira.
  - `cls`, `read` (Bolsa): Limpa a tela e carrega lista de empresas de arquivo para facilitar testes e defesa do trabalho.

- **Comunicação por Pipes com Strings**:
  - Simplifica depuração, interpretação e extensão do protocolo de comunicação.
  - Facilita registro em logs e diagnóstico.

- **Array Vazio de Empresas**:
  - Evita deadlock e problemas de exclusão mútua ao iniciar o Board sem empresas registradas.

---

## Tabela dos Requisitos Implementados

| ID | Descrição                              | Estado       |
|----|----------------------------------------|--------------|
| 1  | Memória partilhada                     | Implementado |
| 2  | Named pipes                            | Implementado |
| 3  | Cliente                                | Implementado |
| 4  | Comandos do Administrador              | Implementado |
| 5  | Comandos do Cliente                    | Implementado |
| 6  | Gravar constante no Registry           | Implementado |
| 7  | Sincronização                          | Implementado |
| 8  | Board                                  | Implementado |
| 9  | BoardGui                               | Implementado |

---

## Como Executar

1. Compile todos os módulos (Bolsa, Board, BoardGUI, Cliente).
2. Inicie o servidor (Bolsa).
3. Execute Board e BoardGUI para visualização das empresas.
4. Inicie os clientes e realize operações de compra, venda e consulta conforme comandos disponíveis.

---

## Observações

- O sistema é destinado a fins acadêmicos e não possui robustez para uso real em produção.
- Funcionalidades extras foram implementadas para facilitar testes e defesa do projeto.

---

## Licença

Este projeto é apenas para fins acadêmicos.

