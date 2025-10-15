#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h> 
#include <io.h>
#include "Bolsa.h"



//--------THREADS BOLSA-----------------------------------//

DWORD WINAPI trataCli(LPVOID data) {
	TDATA* main = (TDATA*)data;
	//_tprintf(TEXT("\n[trataCli]main POS[%d] NCLI[%d]...... (ReadFile)\n"), main->idLoc, *main->nCLi);
	HANDLE hPipesloc;
	DWORD n, id, ret;
	TCHAR pedido[100];
	TCHAR resposta[4000];  // Buffer grande para a resposta
	Cliente recebCli;
	DWORD notificaTempo;
	TCHAR pri[25] = { '\0' };
	TCHAR sec[25] = { '\0' };
	TCHAR tre[25] = { '\0' };

	WaitForSingleObject(main->hMutexC, INFINITE);
	id = main->idLoc;
	Cliente* utilizador = &main->clientes[id];
	hPipesloc = utilizador->hpipe;
	utilizador->carteira.numEmpresasCarteira = 0;
	ReleaseMutex(main->hMutexC);

	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	utilizador->ligado = 0;
	utilizador->continua = 1;
	recebCli.ligado = 0;

	do {
		ZeroMemory(&ov, sizeof(ov));
		ov.hEvent = hEv;

		ret=ReadFile(hPipesloc, pedido, sizeof(pedido), &n, &ov);
		if (ret) {
			//_tprintf(TEXT("\n[trataCli] li de imediato...... (ReadFile)\n"));
		}
		else {
			if (GetLastError() == ERROR_IO_PENDING) {
			//	_tprintf(TEXT("\n[trataCli] Agendei a leitura...... (ReadFile)\n"));
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(hPipesloc, &ov, &n, FALSE);
			}
			else {
				_tprintf(TEXT("[trataCli-ERRO=%d] %d %d... (ReadFile)\n"), GetLastError(), ret, n);
				break;
			}
		}
		pedido[n / sizeof(TCHAR)] = _T('\0');

		int argumento = _stscanf_s(pedido, _T("%24s %24s %24s"), pri, (unsigned)_countof(pri), sec, (unsigned)_countof(sec), tre, (unsigned)_countof(tre));
		//_tprintf(TEXT("argumento %d pri:%s |sec:%s |tre:%s\n"), argumento, pri, sec, tre);

		// Inicializa o buffer resposta
		ZeroMemory(resposta, sizeof(resposta));
		if (main->pauseComando > 0) {
			DWORD tempoAtual = GetTickCount();
			notificaTempo = (tempoAtual - main->instantePause) / 1000;
			//_tprintf_s(_T("\ninstance:%u tempoAtual:%u tempoRestante:%u PAUSE:%d\n"), main->instantePause, tempoAtual, notificaTempo, main->pauseComando);
			if ((tempoAtual - main->instantePause) < (main->pauseComando * 1000)) {
				_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Compras e vendas estão pausadas."));
			}
			else {
				main->pauseComando = 0; // Reseta a pausa após o tempo definido
			}
		}

		if (_tcscmp(pri, _T("login")) == 0) {
			if (utilizador->ligado) {
				_tcscpy_s(resposta, 100, _T("Já estás Online."));
			}
			else {
			/*	wcscpy_s(recebCli.nome, 100, sec);
				wcscpy_s(recebCli.senha, 100, tre);
				*/
				wcscpy_s(utilizador->nome, 100, sec);
				wcscpy_s(utilizador->senha, 100, tre);

				switch(ValidateCliente(utilizador,*main)) {
				case 1:
				/*	_tcscpy_s(utilizador->nome, TAM_STR, recebCli.nome);
					utilizador->saldo = recebCli.saldo;
					_tcscpy_s(utilizador->senha, TAM_STR, recebCli.senha);
					utilizador->ligado = recebCli.ligado;*/
					_tcscpy_s(resposta, 100, TEXT("Foste aceite"));
					_tprintf_s(_T("\nUtilizador:%s entrou\n"), utilizador->nome);
					break;
				case 2:
					_tcscpy_s(resposta, 100, _T("Ja existe esse utilizador Online.Tente Outro"));break;
				default:
					_tcscpy_s(resposta, 100, _T("Não Foste aceite.Tente Novamente\n"));
				}
			}
		}
		else if (_tcscmp(pri, _T("exit")) == 0) {
			_tcscpy_s(resposta, 100, _T("exit"));
			utilizador->continua = 0;
			updateClientBalance(utilizador);
		}
		else if (!utilizador->ligado) {
			_tcscpy_s(resposta, 100, _T("Faça login Primeiro."));
		}
		else if (_tcscmp(pri, _T("listc")) == 0) {
			listarEmpresas(main->empresas, *main->numEmpresas, resposta, sizeof(resposta) / sizeof(TCHAR));
		}
		else if (_tcscmp(pri, _T("balance")) == 0) {
			_stprintf_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Saldo: %.2f"), utilizador->saldo);
		}
		else if (_tcscmp(pri, _T("buy")) == 0) {

			if (main->pauseComando > 0) {

				_stprintf_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("------Compras e vendas estão pausadas\t |Cronometro: %u \t| Pause:%u-----\n"), notificaTempo, main->pauseComando);
			}
			else {
				int nAcoes = _tstoi(tre);
				if (nAcoes == 0) {
					//	_tprintf_s(TEXT("Número inválido de ações\n"));
					_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), TEXT("Número inválido de ações\n"));
				}
				else {
					WaitForSingleObject(main->hMutexC, INFINITE);
					DWORD resp = Compra(utilizador, main->empresas, *main->numEmpresas, sec, nAcoes);
					switch (resp)
					{
					case 0:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Carteira cheia. Não é possível adicionar mais empresas.\n"));break;
					case 1:
						_stprintf_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Compraste %d ações de %s"), nAcoes, sec);break;
					case 2:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Ações insuficientes na carteira.\n"));break;
					case 3:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Empresa não encontrada.\n"));break;
					default:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Falha ao comprar ações."));
						break;
					}

					ReleaseMutex(main->hMutexC);
				}
			}

		}
		else if (_tcscmp(pri, _T("sell")) == 0) {
			if (main->pauseComando > 0) {
				_stprintf_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("------Compras e vendas estão pausadas\t |Cronometro: %u \t| Pause:%u-----\n"), notificaTempo, main->pauseComando);
			}
			else {

				int nAcoes = _tstoi(tre);
				if (nAcoes == 0) {
					//	_tprintf_s(TEXT("Número inválido de ações\n"));
					_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), TEXT("Número inválido de ações\n"));
				}
				else {
					WaitForSingleObject(main->hMutexC, INFINITE);
					DWORD resp = Venda(utilizador, main->empresas, *main->numEmpresas, sec, nAcoes);
					switch (resp)
					{
					case 0:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Sua carteira está vazia. Realize uma compra primeiro."));break;
					case 1:
						_stprintf_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Vendeste %d ações de %s"), nAcoes, sec);break;
					case 2:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Ações insuficientes na carteira.\n"));break;
					case 3:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Empresa não encontrada na sua carteira.\n"));break;
					case 4:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Empresa não encontrada na sua carteira.\n"));break;
					default:
						_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Falha ao comprar ações."));
						break;
					}
					ReleaseMutex(main->hMutexC);
				}
			}
		}
		else if (_tcscmp(pri, _T("carteira")) == 0) {  // Novo comando para listar a carteira
			listarCarteira(utilizador, resposta, sizeof(resposta) / sizeof(TCHAR));
		}
		else {
			_tcscpy_s(resposta, sizeof(resposta) / sizeof(TCHAR), _T("Comando inválido\n"));
		}

		//_tprintf(TEXT("\n[trataCli] vai validar %d\n"), utilizador->ligado);

		ZeroMemory(&ov, sizeof(ov));
		ov.hEvent = hEv;
		ret=WriteFile(hPipesloc, resposta, (DWORD)(_tcslen(resposta) + 1) * sizeof(TCHAR), &n, &ov);
		if (ret) {
			//_tprintf(TEXT("[trataCli] escrevi de imediato...... (WriteFile)\n"));
		}
		else {
			if (GetLastError() == ERROR_IO_PENDING) {
			//	_tprintf(TEXT("[trataCli] Agendei a escrita...... (WriteFile)\n"));
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(hPipesloc, &ov, &n, FALSE);
			}
			else {
				_tprintf(TEXT("[trataCli-ERRO:%d] %d %d... (WriteFile)\n"), GetLastError(), ret, n);
				break;
			}
		}

		//	_tprintf(TEXT("[trataCli] Enviei %d bytes ao cliente %d continua :%d...NCLI[%d] (WriteFile)\n"), n, id, utilizador->continua, *main->nCLi);
	} while (utilizador->continua == 1);
	//_tprintf(_T("\n TrataCli Bolsa fechando... %d\n"), main->continua ? 1 : 0);
	//_tprintf(TEXT("[trataCli] Desligar o pipe %d (DisconnectNamedPipe)\n"), id);
	_tprintf_s(_T("\nUtilizador:%s entrou\n"), utilizador->nome);

	//_tprintf(TEXT("[trataCli] ID: %d nCli[%d] continua :%d... (WriteFile)\n"), id, *main->nCLi, utilizador->continua);

	FlushFileBuffers(hPipesloc);
	if (!DisconnectNamedPipe(hPipesloc)) {
		_tprintf(TEXT("[trataCli-ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
		exit(-1);
	}
	CloseHandle(hEv);
	CloseHandle(hPipesloc);

	ReleaseSemaphore(main->hSem, 1, NULL);

	WaitForSingleObject(main->hMutexC, INFINITE);
	(*(main->nCLi))--;
	main->hPipes[id] = NULL;
	ReleaseMutex(main->hMutexC);
	//_tprintf(TEXT("[trataCli] antes de sair nCli[%d] \n"), *main->nCLi);
	return 0;
}


DWORD WINAPI ServerThread(LPVOID lpParam) {
	TDATA* main = (TDATA*)lpParam;
	HANDLE hPipe, hSem, hMutexC;
	DWORD nCli = 0;
	HANDLE hThreadCli[TAM_CLIENTES];

	hMutexC = CreateMutex(NULL, FALSE, NULL);
	hSem = CreateSemaphore(NULL, main->nCliente, main->nCliente, NULL);

	if (!hMutexC || !hSem) {
		_tprintf(TEXT("Falha ao criar mutex ou semáforo.\n"));
		if (hMutexC) CloseHandle(hMutexC);
		if (hSem) CloseHandle(hSem);
		return 1;
	}

	main->hMutexC = hMutexC;
	main->hSem = hSem;

	_tprintf(_T("\nServThread continua... %d\n"), main->continua ? 1 : 0);
	do {
		if (WaitForSingleObject(hSem, INFINITE) == WAIT_FAILED) {
			_tprintf(TEXT("Erro ao esperar pelo semáforo.\n"));
			continue;
		}

		//	_tprintf(TEXT("[BOLSA] Criar cópia %d do pipe '%s' ... (CreateNamedPipe)\n"), nCli, PIPE_NAME);
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, sizeof(Cliente), sizeof(Cliente), 1000, NULL);

		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)\n"));
			ReleaseSemaphore(hSem, 1, NULL);
			continue;
		}

		//	_tprintf(TEXT("[BOLSA] Esperar ligação de um cliente %d... (ConnectNamedPipe)\n"), nCli);
		BOOL conectar = ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED;
		if (!conectar) {
			_tprintf(TEXT("[ERRO] Ligação ao leitor! (ConnectNamedPipe)\n"));
			CloseHandle(hPipe);
			ReleaseSemaphore(hSem, 1, NULL);
			continue;
		}

		WaitForSingleObject(hMutexC, INFINITE);
		if (nCli < main->nCliente) {

			for (DWORD i = 0; i < main->nCliente; i++)
			{
				if (main->hPipes[i] == NULL) {
					main->clientes[i].hpipe = hPipe;
					main->hPipes[i] = hPipe;
					main->nCLi = &nCli;
					main->idLoc = i;
					break;
				}

			}
			//SERVE PARA VERIFICAR SE OS PIPES ESTÃO A SERVER BEM MANIPULADOS
		/*	for (size_t i = 0; i < main->nCliente; i++)
			{
				_tprintf(TEXT("\n[HPIPE] %d\n"), main->hPipes[i] == NULL ? 0 : 1);
			}*/

			//_tprintf(TEXT("\n[SERVERCLIENTE] IDLOC:%d NCLI[%d] NUM:%d\n"), main->idLoc, *main->nCLi, *main->numEmpresas);
			hThreadCli[nCli] = CreateThread(NULL, 0, trataCli, (LPVOID)(main), 0, NULL);
			if (!hThreadCli[nCli]) {
				_tprintf(TEXT("[ERRO] Não foi possível criar a thread do cliente!\n"));
				main->clientes[nCli].hpipe = NULL;
				DisconnectNamedPipe(hPipe);
				CloseHandle(hPipe);
				ReleaseSemaphore(hSem, 1, NULL);
			}
			else {
				nCli++;
			}
		}
		else {
			_tprintf(TEXT("Número máximo de clientes atingido. Desconectando...\n"));
			DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
			ReleaseSemaphore(hSem, 1, NULL);
		}
		ReleaseMutex(hMutexC);

	} while (main->continua);

	_tprintf(_T("\nServThread fechando... %d\n"), main->continua ? 1 : 0);
	for (DWORD i = 0; i < nCli; i++) {
		if (main->hPipes[i] != NULL)
		{
			DisconnectNamedPipe(main->hPipes[i]);
		}

		WaitForSingleObject(hThreadCli[i], INFINITE);
		CloseHandle(hThreadCli[i]);
	}
	CloseHandle(hSem);
	CloseHandle(hMutexC);
	return 0;
}

int _tmain(int argc, TCHAR* argv[]) {

	HANDLE MutexBolsa = CreateMutex(NULL, TRUE, _T("ProcessoUnico"));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf_s(_T("Outra instancia da bolsa ja esta em execuo.\n"));
		return 1;
	}


	//Variáveis para tratar do mapeamento e da Thread
	HANDLE hMap, hThread;
	TDATA td;
	//Varíaveis para tratar de comandos
	TCHAR inputLine[256];
	TCHAR* context = NULL;
	TCHAR* token;
	//Variáveis para tratar dos comandos addc, lerEmpresas,...
	TCHAR nomeEmpresa[TAM_STR];
	TCHAR nomeFich[TAM_STR];
	DWORD numeroAcoes = 0;
	DWORD precoAcao = 0;
	DWORD numEmpresas = 0;
	//Array de Clientes
	Cliente clientes[TAM_CLIENTES];
	td.pauseComando = 0;
	td.numEmpresas = &numEmpresas;

	// Ler o valor de NCLIENTES do registro e atribuir a td.nCliente
	td.nCliente = LerNClientesDoRegistro(MAX, TAM_CLIENTES);

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif 

	td.hMutex = CreateMutex(NULL, FALSE, NOME_MUTEX_IN);
	td.hEv = CreateEvent(NULL, TRUE, TRUE, _T("Event"));
	hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TDATA), _T("Empresa"));
	if (hMap == NULL) {
		_tprintf(_T("Erro ao criar o mapeamento do arquivo.\n"));
		return 1;
	}

	_tprintf(_T("\nBOLSA A COMEÇAR... Escreva 'close' para terminar...\n"));
	td.continua = TRUE;

	hThread = CreateThread(NULL, 0, WriterMemory, &td, 0, NULL);

	//cria a THREAD que trata clientes
	for (DWORD i = 0; i < TAM_CLIENTES - 1; i++)
	{
		td.hPipes[i] = NULL;
	}

	td.clientes = clientes;
	HANDLE hServerThread = CreateThread(NULL, 0, ServerThread, &td, 0, NULL);

	do {
		_tprintf(_T("[BOLSA]> Digite um comando: "));
		_fgetts(inputLine, sizeof(inputLine) / sizeof(inputLine[0]), stdin);

		size_t ln = _tcslen(inputLine) - 1;
		if (inputLine[ln] == '\n')
			inputLine[ln] = '\0';

		token = _tcstok_s(inputLine, _T(" "), &context);
		if (token != NULL && _tcscmp(token, _T("addc")) == 0) {
			token = _tcstok_s(NULL, _T(" "), &context);
			if (token != NULL) {
				_tcscpy_s(nomeEmpresa, sizeof(nomeEmpresa) / sizeof(nomeEmpresa[0]), token);
				token = _tcstok_s(NULL, _T(" "), &context);
				if (token != NULL) {
					numeroAcoes = _tstoi(token);
					token = _tcstok_s(NULL, _T(" "), &context);
					if (token != NULL) {
						precoAcao = _tstoi(token);
						addc(td.empresas, &numEmpresas, nomeEmpresa, precoAcao, numeroAcoes);
					}
				}
			}

		}
		else if (token != NULL && _tcscmp(token, _T("listc")) == 0) {
			listc(td.empresas, numEmpresas);
		}
		else if (token != NULL && _tcscmp(token, _T("stock")) == 0) {
			token = _tcstok_s(NULL, _T(" "), &context);
			if (token != NULL) {
				_tcscpy_s(nomeEmpresa, sizeof(nomeEmpresa) / sizeof(nomeEmpresa[0]), token);
				token = _tcstok_s(NULL, _T(" "), &context);
				if (token != NULL) {
					precoAcao = _tstoi(token);
					stock(td.empresas, nomeEmpresa, precoAcao, &numEmpresas);

				}
			}

		}
		else if (token != NULL && _tcscmp(token, _T("read")) == 0) {


			_tcscpy_s(nomeFich, sizeof(nomeFich) / sizeof(nomeFich[0]), _T("empresas.txt"));
			lerFicheiroEmpresas(td.empresas, nomeFich, &numEmpresas);

			//_tprintf_s(_T("\nfora NUM:%d"), *td.numEmpresas);

		}
		else if (token != NULL && _tcscmp(token, _T("close")) == 0) {
			td.continua = FALSE;

		}
		else if (token != NULL && _tcscmp(token, _T("users")) == 0) {
			users(td);
		}
		else if (token != NULL && _tcscmp(token, _T("pause")) == 0) {
			token = _tcstok_s(NULL, _T(" "), &context);
			if (token != NULL) {
				td.pauseComando = _tstoi(token);
				td.instantePause = GetTickCount();
			}

		}
		else if (token != NULL && _tcscmp(token, _T("cls")) == 0) {
			system("cls");
		}
	} while (_tcscmp(inputLine, _T("close")) != 0);


	_tprintf(_T("\nBolsa fechando... %d\n"), td.continua ? 1 : 0);


	WaitForSingleObject(hThread, INFINITE);
	//WaitForSingleObject(hServerThread, INFINITE);
	CloseHandle(td.hEv);
	CloseHandle(td.hMutex);
	CloseHandle(hMap);
	CloseHandle(MutexBolsa);
	return 0;
}
