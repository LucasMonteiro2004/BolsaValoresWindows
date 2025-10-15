#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h> 
#include <io.h>

#define TAM 30
#define TAM_STR 100
#define MAX_EMPRESAS 30
#define NEMPRESAS_DISPLAY 10
#define NOME_SM		    _T("memória")
#define NOME_MUTEX_IN   _T("mutex_in")
#define NOME_MUTEX_OUT  _T("mutex_out")

typedef struct {
	TCHAR nomeEmp[TAM_STR];
	DWORD valorAcao;
	DWORD numAcoes;
	TCHAR UltimaTransicao[TAM_STR];
} Empresa;

typedef struct {
	BOOL continua;
	DWORD nDisplayEmp;
	HANDLE hMutex, hEv;
	Empresa empresas[MAX_EMPRESAS];
	DWORD pauseComando;
} TDATA;

DWORD WINAPI atualizaTopEmpresas(LPVOID data) {

	TDATA* td = (TDATA*)data;
	DWORD i = 0, pos = 0;
	HANDLE hEv = OpenEvent(EVENT_MODIFY_STATE, FALSE, _T("Event"));
	HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, _T("Mutex"));
	Empresa* pEmpresas;
	do {
		

		td->pauseComando = 0;
		WaitForSingleObject(hEv, INFINITE);
		WaitForSingleObject(hMutex, INFINITE);

		HANDLE hMap = OpenFileMapping(FILE_MAP_WRITE, FALSE, _T("Empresa"));
		pEmpresas = (Empresa*)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(Empresa));

		system("cls");
		_tprintf_s(_T("Empresa\t\t\tAções Disponiveis\t\t\tPreço Ação\n\n"));

		// Mostrando o vetor de empresas na tela
		for (int i = 0; i < td->nDisplayEmp; i++) {
			_tprintf(TEXT("%s \t\t\ | \t\t %d \t\t | \t\t %u \n"), pEmpresas[i].nomeEmp, pEmpresas[i].numAcoes, pEmpresas[i].valorAcao);
		}
		_tprintf_s(_T("\n-----------------------ULTIMAS TRANSACOES---------------------\n"));
		// Mostrando ultima transição
		 // Mostrando a última transação geral
		TCHAR ultimaTransacao[TAM_STR] = _T("");
		
		for (int i = 0; i < td->nDisplayEmp; i++) {
			if (_tcscmp(pEmpresas[i].UltimaTransicao, _T(" ")) != 0) {
				_tcscpy_s(ultimaTransacao, TAM_STR, pEmpresas[i].UltimaTransicao);
	
			}
		}
		_tprintf_s(_T("%s\n"), ultimaTransacao);


		Sleep(1000);
		UnmapViewOfFile(pEmpresas);
		ReleaseMutex(hMutex);
		CloseHandle(hMap);

	} while (td->continua);

	_tprintf_s(_T("\n [BOARD] fechando...	\n"));

	CloseHandle(hEv);
	CloseHandle(hMutex);

	return 0;
}

int _tmain(int argc, TCHAR* argv[]) {

	HANDLE hMap, hThread, hSem;
	TDATA td;
	TDATA* pData;
	TCHAR str[40];
	BOOL primeiro = FALSE;
	DWORD displays = 0;
	td.pauseComando = 0;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif 

	if (argc > 1) {
		displays = _tstoi(argv[1]);
	}
	else {
		displays = 10;
	}

	hSem = CreateSemaphore(NULL, MAX_EMPRESAS, MAX_EMPRESAS, _T("Sem"));
	//Esperar No semaforo (-1)
	WaitForSingleObject(hSem, INFINITE);

	hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TDATA), NOME_SM);
	if (hMap != NULL && GetLastError() != ERROR_ALREADY_EXISTS) {
		primeiro = TRUE;
	}
	pData = (TDATA*)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	//INICIAR
	td.hMutex = CreateMutex(NULL, FALSE, NOME_MUTEX_IN);
	td.hEv = CreateEvent(NULL, TRUE, FALSE, _T("Event"));
	td.nDisplayEmp = displays;
	td.continua = TRUE;

	hThread = CreateThread(NULL, 0, atualizaTopEmpresas, &td, 0, NULL);
	do {
		_tscanf_s(_T("%s"), str, 40 - 1);
		_tprintf(_T("Estou no loop"));
	} while (_tcscmp(str, _T("fim")) != 0 && td.continua==TRUE);
	td.continua = FALSE;
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(td.hMutex);
	CloseHandle(td.hEv);
	UnmapViewOfFile(pData);
	CloseHandle(hMap);
	ReleaseSemaphore(hSem, 1, NULL);
	_tprintf(_T("Board fechando...%d\n"), td.continua ? 1 : 0);
	return 0;
}