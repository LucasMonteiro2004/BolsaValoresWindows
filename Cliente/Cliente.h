#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h> 
#include <io.h>

#define MAX_USER 20
#define MAX 5
#define MAX_EMPRESAS 30
#define TAM_CLIENTES 20
#define NEMPRESAS_DISPLAY 10
#define TAM_STR 100
#define NOME_SM		    _T("sharedMemory")
#define PIPE_NAME TEXT("\\\\.\\pipe\\pipeComunica")
#define SEM_O TEXT("semaforo_cliente")
#define NOME_MUTEX_IN   _T("mutex_in")
#define NOME_MUTEX_OUT  _T("mutex_out")


typedef struct {
    TCHAR nomeEmp[TAM_STR];
    DWORD valorAcao;
    DWORD numAcoes;
} Empresa;

typedef struct {
    Empresa emp[MAX_EMPRESAS];
} CarteiraAcoes;


typedef struct {
    TCHAR nome[TAM_STR];
    FLOAT saldo;
    TCHAR senha[TAM_STR];
    DWORD ligado; //flag para ver se está ativo ou não
    DWORD continua;
    HANDLE hpipe, hMutexC, hSem;
    CarteiraAcoes carteira;
    DWORD* id;
    DWORD pedido;
    TCHAR cmd[100];
    TCHAR buffer[4000];
} Cliente;


typedef struct {
    BOOL continua;
    HANDLE hMutex, hMutexC, hEv;
    Empresa empresas[MAX_EMPRESAS];
    DWORD pauseComando;
    Cliente clientes[TAM_CLIENTES];
} TDATA;
