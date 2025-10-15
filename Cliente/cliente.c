#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <time.h>
#include "Cliente.h"



DWORD WINAPI leMsg(LPVOID data) {
    //[1]
    Cliente *tCliente= (Cliente*)data;
    HANDLE hPipe = tCliente->hpipe;
    TCHAR resposta[4000];
    int i = 0;
    BOOL ret;
    DWORD n;
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

    do
    {
        //[2]
        ZeroMemory(&ov, sizeof(ov));
        ov.hEvent = hEv;
        ret = ReadFile(hPipe, resposta, sizeof(resposta), &n, &ov);

        if (ret) {
         //   _tprintf(TEXT("\n[CLIENTE] li de imediato...... (ReadFile)\n"), ret, n);
        }
        else
        {
            if (GetLastError() == ERROR_IO_PENDING) {
            //    _tprintf(TEXT("\n[CLIENTE] Agendei a leitura...... (ReadFile)\n"), ret, n);
                //..
                WaitForSingleObject(hEv, INFINITE);
                GetOverlappedResult(hPipe, &ov, &n, FALSE);
            }
            else {
                _tprintf(TEXT("[ERRO] %d %d... (ReadFile)\n"), ret, n);
                exit(3);
                break;
            }

        }
        resposta[n / sizeof(TCHAR)] = _T('\0');
      //  _tprintf(TEXT("[leMsg] Recebi %d bytes (ReadFile)\n"), n);
   
        system("CLS");
        _tprintf(TEXT("\n----------------------RESPOSTA----------------------------\n %s\n"), resposta);
        _tprintf(TEXT("\n[CLIENTE]<#comando> "));
        if (_tcscmp(resposta, _T("exit")) == 0) {

            _tprintf(TEXT("\n[leMsg] encerrar...\n"));
        }


    } while (_tcscmp(resposta, _T("exit")) != 0 && tCliente->continua == 1);

    _tprintf(TEXT("[leMSG] desligar pipe\n"));
    CloseHandle(hPipe);

    return 0;

}

int _tmain(int argc, LPTSTR argv[]) {
    TCHAR cmd[100];
    HANDLE hPipe, hThread;
    int i = 0;
    BOOL ret;
    DWORD n;
    Cliente cliente;

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif

    //_tprintf(TEXT("[CLIENTE] Esperar pelo pipe '%s' (WaitNamedPipe)\n"),
    _tprintf(TEXT("[CLIENTE] Aguarde pela ligação\n"),
        PIPE_NAME);
    if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf(TEXT("[CLIENTE] Ligaçaoo ao pipe da bolsa... (CreateFile)\n"));
    hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    if (hPipe == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
        exit(-1);
    }

    _tprintf(TEXT("[CLIENTE] Ja se encontra ligado...\n"));

    cliente.hpipe = hPipe;
    cliente.ligado = 0;
    cliente.continua = 1;
    //cria thread
    hThread = CreateThread(NULL, 0, leMsg, &cliente, 0, NULL);
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

    //pede login


    _tprintf(TEXT("[CLIENTE]FAÇA LOGIN \n"));
    do {
   
        _tprintf(TEXT("[CLIENTE]<#comando> "));
        _fgetts(cmd, 100, stdin);
         cmd[_tcslen(cmd) - 1] = _T('\0');

   _tprintf_s(TEXT("\n[CLIENTE]cmd:%s\n"), cmd);
        //[2]
        ZeroMemory(&ov, sizeof(ov));
        ov.hEvent = hEv;

        ret = WriteFile(hPipe, cmd, (DWORD)_tcslen(cmd) * sizeof(TCHAR), &n, &ov);

        if (ret) {
         //   _tprintf(TEXT("[CLIENTE] escrevi de imediato...... ( WriteFile)\n"));
        }
        else
        {
            if (GetLastError() == ERROR_IO_PENDING) {
            //    _tprintf(TEXT("[CLIENTE] Agendei a escrita...... ( WriteFile)\n"));
                //..
                WaitForSingleObject(hEv, INFINITE);
                GetOverlappedResult(hPipe, &ov, &n, FALSE);
            }
            else {
                _tprintf(TEXT("[ERRO] %d %d... ( WriteFile)\n"), ret, n);
                exit(3);
                break;
            }

        }
      //  _tprintf(TEXT("[CLIENTE] Escrevi %d bytes: '%s'... ( WriteFile)\n"), n, cmd);
    } while (_tcscmp(cmd, _T("exit")) != 0 && cliente.continua == 1);
    _tprintf(TEXT("[CLIENTE] estou a espera do thread[leMsg]\n"));
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
}