#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h> 
#include <io.h>


// Definição de constantes

#define MAX 5
#define MAX_EMPRESAS 30
#define TAM_CLIENTES 20
#define NEMPRESAS_DISPLAY 10
#define TAM_STR 100
#define NOME_SM _T("sharedMemory")
#define PIPE_NAME TEXT("\\\\.\\pipe\\pipeComunica")
#define SEM_O TEXT("semaforo_cliente")
#define NOME_MUTEX_IN _T("mutex_in")
#define NOME_MUTEX_OUT _T("mutex_out")

// Estrutura para representar uma empresa
typedef struct {
    TCHAR nomeEmp[TAM_STR];  // Nome da empresa
    DWORD valorAcao;         // Valor da ação
    DWORD numAcoes;          // Número de ações disponíveis
    TCHAR UltimaTransicao[TAM_STR]; // String da ultima tramsição
} Empresa;

// Estrutura para representar uma carteira de ações de um cliente
typedef struct {
    Empresa emp[MAX_EMPRESAS];  // Array de empresas na carteira
    DWORD numEmpresasCarteira;
} CarteiraAcoes;

// Estrutura para representar um cliente
typedef struct {
    TCHAR nome[TAM_STR];       // Nome do cliente
    FLOAT saldo;               // Saldo do cliente
    TCHAR senha[TAM_STR];      // Senha do cliente
    DWORD ligado;              // Flag para indicar se está ativo
    DWORD continua;            // Flag para indicar se continua conectado
    HANDLE hpipe;              // Handle do pipe de comunicação
    CarteiraAcoes carteira;    // Carteira de ações do cliente
    TCHAR cmd[100];            // Comando atual do cliente
} Cliente;

// Estrutura para armazenar dados compartilhados
typedef struct {
    BOOL continua;             // Flag para indicar se o servidor continua rodando
    HANDLE hMutex, hMutexC, hSem, hEv; // Handles para sincronização
    Empresa empresas[MAX_EMPRESAS];    // Array de empresas
    DWORD* numEmpresas;        // Ponteiro para o número de empresas
    DWORD pauseComando;        // Tempo de pausa entre comandos
    DWORD instantePause;
    Cliente* clientes;         // Array de clientes
    DWORD idLoc;               // ID do cliente local
    DWORD* nCLi;               // Ponteiro para o número de clientes
    DWORD nCliente;
    HANDLE hPipes[TAM_STR];
} TDATA;

void updateClientBalance(Cliente* cliente);

DWORD LerNClientesDoRegistro(DWORD maxClientes, DWORD tamClientes);
// Função para realizar a compra de ações
DWORD Compra(Cliente* cliente, Empresa* empresas, DWORD numEmpresas, TCHAR* nomeEmpresa, DWORD numAcoes);

// Função para realizar a venda de ações
DWORD Venda(Cliente* cliente, Empresa* empresas, DWORD numEmpresas, TCHAR* nomeEmpresa, DWORD numAcoes);

// Função para validar um cliente a partir de um arquivo de lista de usuários
INT ValidateCliente(Cliente* cliente, TDATA td);

// Função para verificar se o array de ações está inicializado
int isStockArrayInitialized(Empresa* stocks, DWORD numEmpresas);

// Função para ordenar as ações por valor
void sortStocks(Empresa* empresas, DWORD numEmpresas);

// Função para ler clientes de um arquivo
void lerFicheiroClientes(Cliente* clientes, TCHAR* nomeFich, DWORD* numClientes);

// Função para ler empresas de um arquivo
void lerFicheiroEmpresas(Empresa* empresas, TCHAR* nomeFich, DWORD* numEmpresas);

// Função para adicionar uma nova empresa
void addc(Empresa* emp, DWORD* numEmpresas, TCHAR* nomeEmpresa, DWORD precoAcao, DWORD numAcoes);

// Função para listar todas as empresas registradas
void listc(Empresa* emp, DWORD numEmpresas);

// Função para atualizar o valor de uma ação de uma empresa
void stock(Empresa* emp, TCHAR* nomeEmpresa, DWORD precoAcao, DWORD* numEmpresas);

// Função para listar os usuários conectados
void users(TDATA td);

void listarEmpresas(Empresa* empresas, DWORD numEmpresas, TCHAR* resposta, size_t respostaSize);

void listarCarteira(Cliente* cliente, TCHAR* resposta, size_t respostaSize);



// Função para comunicação do servidor com o painel de controle
DWORD WINAPI WriterMemory(LPVOID data);
