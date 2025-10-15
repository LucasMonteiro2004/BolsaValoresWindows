#include <stdio.h>
#include <fcntl.h> 
#include <io.h>
#include "Bolsa.h"


DWORD LerNClientesDoRegistro(DWORD maxClientes, DWORD tamClientes) {
	HKEY chave;
	DWORD nClientes = 5, dataSize = sizeof(DWORD);
	LONG lResult;
	DWORD erro;

	// Criação ou Abertura do Registry com NCLIENTES
	lResult = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\TPSO2\\NCLIENTES", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &chave, &erro);

	if (lResult != ERROR_SUCCESS) {
		_tprintf(_TEXT("Erro ao criar/abrir chave no Registry (%d)\n"), GetLastError());
		return maxClientes;
	}

	// Ler o valor DWORD do registry
	lResult = RegQueryValueExA(chave, "NCLIENTES", NULL, NULL, (LPBYTE)&nClientes, &dataSize);
	if (lResult != ERROR_SUCCESS || nClientes < 5) {
		// Se não conseguir ler ou valor inválido, define para DEFAULT_NCLIENTES
		nClientes = maxClientes;
		RegSetValueExA(chave, "NCLIENTES", 0, REG_DWORD, (const BYTE*)&nClientes, sizeof(DWORD));
	}
	else if (nClientes > tamClientes) {
		// Limitar NCLIENTES ao valor máximo permitido
		nClientes = tamClientes;
		RegSetValueExA(chave, "NCLIENTES", 0, REG_DWORD, (const BYTE*)&nClientes, sizeof(DWORD));
	}

	RegCloseKey(chave);
	return nClientes;
}
DWORD Compra(Cliente* cliente, Empresa* empresas, DWORD numEmpresas, TCHAR* nomeEmpresa, DWORD numAcoes) {
	for (DWORD i = 0; i < numEmpresas; i++) {
		if (_tcscmp(empresas[i].nomeEmp, nomeEmpresa) == 0) {
			DWORD totalPreco = numAcoes * empresas[i].valorAcao;
			if (cliente->saldo >= totalPreco && empresas[i].numAcoes >= numAcoes) {


				BOOL empresaNaCarteira = FALSE;
				for (DWORD j = 0; j < cliente->carteira.numEmpresasCarteira; j++) {
					if (_tcscmp(cliente->carteira.emp[j].nomeEmp, nomeEmpresa) == 0) {
						cliente->carteira.emp[j].numAcoes += numAcoes;
						empresaNaCarteira = TRUE;
						break;
					}
				}

				// Verifica se a carteira está cheia antes de tentar adicionar uma nova empresa
				if (!empresaNaCarteira) {
					if (cliente->carteira.numEmpresasCarteira < 5) {
						_tcscpy_s(cliente->carteira.emp[cliente->carteira.numEmpresasCarteira].nomeEmp, TAM_STR, nomeEmpresa);
						cliente->carteira.emp[cliente->carteira.numEmpresasCarteira].numAcoes = numAcoes;
						cliente->carteira.numEmpresasCarteira++;
					}
					else {
						_tprintf(_T("Carteira cheia. Não é possível adicionar mais empresas.\n"));
						return 0;  // Código de erro para carteira cheia
					}
				}
				//altera valores das ações e saldo:
				cliente->saldo -= totalPreco;
				empresas[i].numAcoes -= numAcoes;
				// Aumenta o preço da ação em 50%
				empresas[i].valorAcao = (DWORD)round(empresas[i].valorAcao * 1.40);

				// Limpa as últimas transições de todas as empresas na carteira do cliente
				for (DWORD k = 0; k < numEmpresas; k++) {
					_tcscpy_s(empresas[k].UltimaTransicao, TAM_STR, _T(" "));
				}

				// Registra a transação
				//_tprintf(_T("\nCompra realizada: %s comprou %lu ações de %s por %lu.\n"), cliente->nome, numAcoes, nomeEmpresa, totalPreco);
				_stprintf_s(empresas[i].UltimaTransicao, TAM_STR, _T("Compra: %s comprou %lu ações de %s por %lu.\n"), cliente->nome, numAcoes, nomeEmpresa, totalPreco);

				return 1;
			}
			else {
				_tprintf(_T("Saldo insuficiente ou ações indisponíveis.\n"));
				return 2;
			}
		}
	}
	_tprintf(_T("Empresa não encontrada.\n"));
	return 3;
}

DWORD Venda(Cliente* cliente, Empresa* empresas, DWORD numEmpresas, TCHAR* nomeEmpresa, DWORD numAcoes) {
	// Verifica se a carteira do cliente está vazia
	if (cliente->carteira.numEmpresasCarteira == 0) {
		_tprintf(_T("Sua carteira está vazia. Realize uma compra primeiro.\n"));
		return 0;
	}

	// Procura a empresa na lista de empresas da bolsa
	for (DWORD i = 0; i < numEmpresas; i++) {
		if (_tcscmp(empresas[i].nomeEmp, nomeEmpresa) == 0) {
			// Procura a empresa na carteira do cliente
			for (DWORD j = 0; j < cliente->carteira.numEmpresasCarteira; j++) {
				if (_tcscmp(cliente->carteira.emp[j].nomeEmp, nomeEmpresa) == 0) {
					// Verifica se o cliente possui ações suficientes
					if (cliente->carteira.emp[j].numAcoes >= numAcoes) {
						DWORD totalPreco = numAcoes * empresas[i].valorAcao;
						cliente->saldo += totalPreco;
						empresas[i].numAcoes += numAcoes;
						cliente->carteira.emp[j].numAcoes -= numAcoes;

						// Remove a empresa da carteira se o número de ações for zero
						if (cliente->carteira.emp[j].numAcoes == 0) {
							for (DWORD k = j; k < cliente->carteira.numEmpresasCarteira - 1; k++) {
								cliente->carteira.emp[k] = cliente->carteira.emp[k + 1];
							}
							cliente->carteira.numEmpresasCarteira--;
						}

						// Diminui o preço da ação em 50%
						empresas[i].valorAcao = (DWORD)round(empresas[i].valorAcao * 0.50);



						for (DWORD k = 0; k < numEmpresas; k++) {
							_tcscpy_s(empresas[k].UltimaTransicao, TAM_STR, _T(" "));
						}


						// Registra a transação
						//_tprintf(_T("\nVenda realizada: %s vendeu %lu ações de %s por %lu.\n"), cliente->nome, numAcoes, nomeEmpresa, totalPreco);
						_stprintf_s(empresas[i].UltimaTransicao, TAM_STR, _T("Venda realizada: %s vendeu %lu ações de %s por %lu.\n"), cliente->nome, numAcoes, nomeEmpresa, totalPreco);



						return 1;
					}
					else {
						_tprintf(_T("Ações insuficientes na carteira.\n"));
						return 2;
					}
				}
			}
			_tprintf(_T("Empresa não encontrada na sua carteira.\n"));
			return 3;
		}
	}
	_tprintf(_T("Empresa não encontrada na bolsa.\n"));
	return 4;
}

INT ValidateCliente(Cliente* cliente, TDATA td) {

	FILE* file;
	errno_t erro = _tfopen_s(&file, _T("userlist.txt"), _T("r"));

	if (erro != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo de usuários.\n"));
		return FALSE;
	}

	TCHAR nome[100];
	TCHAR senha[100];
	FLOAT saldo = 0.0f;
	INT clienteValido = 0;
	TCHAR line[100];

	for (DWORD i = 0; i < td.nCliente; i++) {
		if (td.hPipes[i] != NULL)
			if (_tcscmp(td.clientes[i].nome, cliente->nome) == 0 && td.clientes[i].ligado) {
				fclose(file);
				return 2;
			}
	}
		while (_fgetts(line, 100, file) != NULL) {
			if (_stscanf_s(line, _T("%99s %99s %f"), nome, 100, senha, 100, &saldo) == 3) {
				if (_tcscmp(nome, cliente->nome) == 0 && _tcscmp(senha, cliente->senha) == 0) {
					//_tprintf(_T("%s %s %f\n"), nome, senha, saldo);
					clienteValido = 1;
					cliente->ligado = 1;
					cliente->saldo = saldo;
					break;
				}
				else {
					cliente->saldo = 0;
				}
			}
		}

	fclose(file);
	return clienteValido;
}

int isStockArrayInitialized(Empresa* stocks, DWORD numEmpresas) {
	for (DWORD i = 0; i < numEmpresas; i++) {
		if (stocks[i].valorAcao > 0) {
			return 1;
		}
	}
	return 0;
}

void sortStocks(Empresa* empresas, DWORD numEmpresas) {
	DWORD i, j;
	Empresa temp;
	for (i = 0; i < numEmpresas - 1; i++) {
		for (j = 0; j < numEmpresas - i - 1; j++) {
			if (empresas[j].valorAcao > empresas[j + 1].valorAcao) {
				temp = empresas[j];
				empresas[j] = empresas[j + 1];
				empresas[j + 1] = temp;
			}
		}
	}
}

void lerFicheiroClientes(Cliente* clientes, TCHAR* nomeFich, DWORD* numClientes) {
	FILE* file;
	errno_t err;

	err = _tfopen_s(&file, nomeFich, _T("r"));
	if (err != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo!\n"));
		return;
	}

	TCHAR buffer[100];
	int i = 0;
	while (_fgetts(buffer, sizeof(buffer) / sizeof(TCHAR), file) != NULL) {
		_stscanf_s(buffer, _T("%99s %99s %f"), clientes[i].nome, 100, clientes[i].senha, 100, &clientes[i].saldo);
		i++;
	}

	*numClientes = i + 1;

	fclose(file);
}

void lerFicheiroEmpresas(Empresa* empresas, TCHAR* nomeFich, DWORD* numEmpresas) {
	FILE* file;
	errno_t err;

	err = _tfopen_s(&file, nomeFich, _T("r"));
	if (err != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo!\n"));
		return;
	}

	TCHAR buffer[100];
	int i = 0;
	while (_fgetts(buffer, sizeof(buffer) / sizeof(TCHAR), file) != NULL) {
		_stscanf_s(buffer, _T("%99s %d %u"), empresas[i].nomeEmp, 100, &empresas[i].numAcoes, &empresas[i].valorAcao);
		_tcscpy_s(empresas[i].UltimaTransicao, TAM_STR, _T(" "));
		i++;
	}

	*numEmpresas = i;
	_tprintf_s(_T("NUM:%d"), *numEmpresas);
	fclose(file);
}

void addc(Empresa* emp, DWORD* numEmpresas, TCHAR* nomeEmpresa, DWORD precoAcao, DWORD numAcoes) {

	_tcscpy_s(emp[*numEmpresas].nomeEmp, sizeof(emp[*numEmpresas].nomeEmp) / sizeof(emp[*numEmpresas].nomeEmp[0]), nomeEmpresa);

	emp[*numEmpresas].numAcoes = numAcoes;
	emp[*numEmpresas].valorAcao = precoAcao;
	_tcscpy_s(emp[*numEmpresas].UltimaTransicao, TAM_STR, _T(" "));
	(*numEmpresas)++;

	_tprintf(_T("\nEmpresa adicionada: %s, Ações: %lu, Preço por Ação: %lu\n"), nomeEmpresa, numAcoes, precoAcao);
}

void listc(Empresa* emp, DWORD numEmpresas) {
	_tprintf(_T("\nLista de Empresas Registadas:\n"));
	if (numEmpresas == 0)
		_tprintf_s(_T("\n-------------Sem Empresas Registradas----------.\n"));
	for (DWORD i = 0; i < numEmpresas; i++) {
		_tprintf_s(_T("Nº:%d | Empresa: %s \t| Ações: %lu \t| Valor por Ação: %lu\n"), i, emp[i].nomeEmp, emp[i].numAcoes, emp[i].valorAcao);
	}
	_tprintf(_T("\n"));
	_tprintf(_T("\n"));
}

void stock(Empresa* emp, TCHAR* nomeEmpresa, DWORD precoAcao, DWORD* numEmpresas) {
	for (DWORD i = 0; i < MAX_EMPRESAS; i++) {
		if (_tcscmp(emp[i].nomeEmp, nomeEmpresa) == 0) {
			emp[i].valorAcao = precoAcao;
			_tprintf_s(_T("\n(atualizado) Valor alterado com sucesso! Empresa: %s novo Valor: %u\n"), emp[i].nomeEmp, emp[i].valorAcao);
		}
	}
}

void users(TDATA td) {

	_tprintf(_T("\nLista de Utilizadores: \n"));

	FILE* file;
	errno_t erro = _tfopen_s(&file, _T("userlist.txt"), _T("r"));

	if (erro != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo de usuários.\n"));
		return;
	}

	TCHAR nome[100];
	TCHAR senha[100];
	FLOAT saldo = 0.0f;
	TCHAR line[100];
	DWORD i = 0;
	while (_fgetts(line, sizeof(line) / sizeof(line[0]), file) != NULL) {
		if (_stscanf_s(line, _T("%99s %99s %f"), nome, (unsigned)_countof(nome), senha, (unsigned)_countof(senha), &saldo) == 3) {
			int ligado = 0;

			for (DWORD i = 0; i < td.nCliente; i++) {
				if (td.hPipes[i] != NULL)
					if (_tcscmp(td.clientes[i].nome, nome) == 0 && td.clientes[i].ligado) {
						ligado = 1;
						saldo = td.clientes[i].saldo;
						break;
					}
			}

			++i;
			_tprintf(_T("Nº:%d \t| User: %s \t| Saldo: %f \t| Estado: %s \n"), i, nome, saldo, ligado ? _T("ONLINE") : _T("OFFLINE"));
		}
	}

	fclose(file);
	_tprintf(_T("\n"));
}
void listarCarteira(Cliente* cliente, TCHAR* resposta, size_t respostaSize) {
	if (cliente->carteira.numEmpresasCarteira == 0) {
		_tcscpy_s(resposta, respostaSize, _T("Sua carteira está vazia.\n"));
		return;
	}

	_tcscpy_s(resposta, respostaSize, _T("Empresas na sua carteira:\n"));
	for (DWORD i = 0; i < cliente->carteira.numEmpresasCarteira; i++) {
		TCHAR temp[256];
		_stprintf_s(temp, sizeof(temp) / sizeof(TCHAR), _T("Nº:%d \t| Empresa: %s\t| Ações: %lu \t| Valor:%u\n"), i + 1, cliente->carteira.emp[i].nomeEmp, cliente->carteira.emp[i].numAcoes, cliente->carteira.emp[i].valorAcao);
		if (_tcslen(resposta) + _tcslen(temp) < respostaSize / sizeof(TCHAR)) {
			_tcscat_s(resposta, respostaSize / sizeof(TCHAR), temp);
		}
		else {
			_tprintf(TEXT("[listarCarteira] Buffer de resposta cheio, truncando resposta\n"));
			break;
		}
	}
}

void listarEmpresas(Empresa* empresas, DWORD numEmpresas, TCHAR* resposta, size_t respostaSize) {
	_tcscpy_s(resposta, respostaSize, _T("Lista de empresas:\n"));
	if (numEmpresas == 0) {
		_tcscat_s(resposta, respostaSize / sizeof(TCHAR), _T("-------------Sem Empresas Registradas----------.\n"));
	}
	//_tcscpy_s(resposta, respostaSize, _T("-------------Sem Empresas Registradas----------.\n"));
	for (DWORD i = 0; i < numEmpresas; i++) {
		TCHAR temp[256];
		_stprintf_s(temp, sizeof(temp) / sizeof(TCHAR), _T("Empresa: %s, Ações Disponíveis: %lu, Preço por Ação: %d\n"), empresas[i].nomeEmp, empresas[i].numAcoes, empresas[i].valorAcao);
		if (_tcslen(resposta) + _tcslen(temp) < respostaSize / sizeof(TCHAR)) {
			_tcscat_s(resposta, respostaSize / sizeof(TCHAR), temp);
		}
		else {
			_tprintf(TEXT("[listarEmpresas] Buffer de resposta cheio, truncando resposta\n"));
			break;
		}
	}
}


VOID geraEmp(Empresa* emp) {
	for (DWORD i = 0; i < 10; i++) {
		_stprintf_s(emp[i].nomeEmp, _countof(emp[i].nomeEmp), TEXT("....."));
		emp[i].numAcoes = 0;
		emp[i].valorAcao = 0;
		_tcscpy_s(emp[i].UltimaTransicao, TAM_STR, _T(" "));
	}
}

DWORD WINAPI WriterMemory(LPVOID data) {

	TDATA* td = (TDATA*)data;
	DWORD i = 0, pos = 0, contador = 0;
	HANDLE hMap;
	Empresa* pEmpresas;

	do {
		if (isStockArrayInitialized(td->empresas, sizeof(td->empresas) / sizeof(td->empresas[0]))) {
			sortStocks(td->empresas, sizeof(td->empresas) / sizeof(td->empresas[0]));
		}

		/*if (td->pauseComando != 0) {
			Sleep(*(td->pauseComando) * 1000);
			_tprintf(_T("\nBoard em pausa de %u segundos\n"), td->pauseComando);
		}*/

		//*td->pauseComando = 0;

		SetEvent(td->hEv);

		WaitForSingleObject(td->hEv, INFINITE);
		hMap = OpenFileMapping(FILE_MAP_WRITE, FALSE, _T("Empresa"));
		if (hMap == NULL) {
			_tprintf(_T("Erro ao abrir FileMapping.\n"));
			return 1;
		}


		pEmpresas = (Empresa*)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(Empresa));
		if (pEmpresas == NULL) {
			_tprintf(_T("Erro ao mapear a visão do arquivo.\n"));
			CloseHandle(hMap);
			return 1;
		}
		WaitForSingleObject(td->hMutex, INFINITE);
		if (*td->numEmpresas == 0) {
			Empresa* emp = (Empresa*)malloc(sizeof(Empresa) * NEMPRESAS_DISPLAY);
			if (emp == NULL) {
				// Tratamento de erro, caso a alocação de memória falhe.
				fprintf(stderr, "Erro ao alocar memória para emp.\n");
				return 1; // Ou outra forma de lidar com o erro.
			}

			geraEmp(emp); // Certifique-se de que esta função preenche a memória corretamente.
			CopyMemory(pEmpresas, emp, sizeof(Empresa) * NEMPRESAS_DISPLAY);

			// Libere a memória se não for mais necessária.
			free(emp);
		}
		else {
			CopyMemory(pEmpresas, &td->empresas, sizeof(Empresa) * NEMPRESAS_DISPLAY);
		}
		ReleaseMutex(td->hMutex);
		UnmapViewOfFile(pEmpresas);
		ResetEvent(td->hEv);
		Sleep(2000);
	} while (td->continua == TRUE);

	_tprintf(_T("\nThrede Memoria fechando... %d\n"), td->continua ? 1 : 0);
	return 0;
}

void updateClientBalance( Cliente* cliente) {
	FILE* file;
	errno_t err;
	TCHAR line[200];
	TCHAR buffer[10000] = _T(""); // Buffer para armazenar o conteúdo do arquivo
	TCHAR temp[200];

	err = _tfopen_s(&file, _T("userlist.txt"), _T("r"));
	if (err != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo para leitura!\n"));
		return;
	}

	// Ler o conteúdo do arquivo
	while (_fgetts(line, sizeof(line) / sizeof(TCHAR), file) != NULL) {
		TCHAR nome[100], senha[100];
		FLOAT saldo;
		if (_stscanf_s(line, _T("%99s %99s %f"), nome, 100, senha, 100, &saldo) == 3) {
			if (_tcscmp(nome, cliente->nome) == 0) {
				// Atualizar o saldo para o cliente específico
				_stprintf_s(temp, sizeof(temp) / sizeof(TCHAR), _T("%s %s %.2f\n"), nome, senha, cliente->saldo);
				_tcscat_s(buffer, sizeof(buffer) / sizeof(TCHAR), temp);
			}
			else {
				_tcscat_s(buffer, sizeof(buffer) / sizeof(TCHAR), line);
			}
		}
	}

	fclose(file);

	// Escrever o conteúdo atualizado de volta para o arquivo
	err = _tfopen_s(&file, _T("userlist.txt"), _T("w"));
	if (err != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo para escrita!\n"));
		return;
	}
	_fputts(buffer, file);
	fclose(file);
}
