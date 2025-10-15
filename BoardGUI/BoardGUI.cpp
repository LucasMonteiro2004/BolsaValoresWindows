#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define MAX_EMPRESAS 30
#define NEMPRESAS_DISPLAY 10
#define TAM_STR 100

#define IDM_EXIT 100
#define IDM_CONFIGURE 101
#define IDM_ABOUT 102
#define IDM_LUCAS 103
#define IDM_AFONSO 104

#define IDD_CONFIGURE 103
#define IDC_SCALE_MIN 1000
#define IDC_SCALE_MAX 1001
#define IDC_NUM_EMPRESAS 1002

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

HINSTANCE hInst;
TDATA td;
TCHAR nomeEmpresaMaisRecente[TAM_STR] = _T("");
BOOL atualizando = TRUE;
int escalaInferior = 0;
int escalaSuperior = 100;
int a = 10;
int* nEmpresasDisplay = &a;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InputWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI atualizaTopEmpresas(LPVOID data);
void DesenhaGraficoBarras(HDC hdc, RECT rect);
BOOL isDWORD(TCHAR* str);
void MostrarImagem(HDC hdc, RECT rect);
void DesenhaLegenda(HDC hdc, RECT rect, Empresa* empresas, int nEmpresas);
DWORD DisplayConfirmSaveAsMessageBox();

// Função principal do Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = { 0 };
    HWND hWnd;
    MSG msg;

    hInst = hInstance;

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("BoardGUIClass");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    hWnd = CreateWindow(
        wc.lpszClassName,
        _T("Board GUI"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    // Criação do menu
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    HMENU hSubMenu2 = CreatePopupMenu();

    AppendMenu(hSubMenu, MF_STRING, IDM_CONFIGURE, _T("&Configure"));
    AppendMenu(hSubMenu, MF_STRING, IDM_EXIT, _T("Exit"));
    AppendMenu(hSubMenu2, MF_STRING, IDM_LUCAS, _T("&Lucas Monteiro"));
    AppendMenu(hSubMenu2, MF_STRING, IDM_AFONSO, _T("Afonso Silva"));
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, _T("&File"));
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu2, _T("&About"));
    SetMenu(hWnd, hMenu);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Iniciar a thread para atualizar os dados
    td.continua = TRUE;
    CreateThread(NULL, 0, atualizaTopEmpresas, &td, 0, NULL);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Função de processamento da janela
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    DWORD val;

    switch (message) {
    case WM_CREATE:
        SetTimer(hWnd, 1, 2000, NULL);  // Definir um temporizador para atualização
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_EXIT:
            val = MessageBox(hWnd, _T("Queres sair?"), _T("Esperar!"), MB_YESNO | MB_ICONEXCLAMATION);
            if (val == IDYES) {
                PostQuitMessage(0);
            }
            break;
        case IDM_CONFIGURE:
            DisplayConfirmSaveAsMessageBox();
            break;
        case IDM_LUCAS:
            MessageBox(hWnd, _T("Nome:Lucas Erardo Alves da Graça Monteiro\n NºAluno:2022153271"), _T("Informação"), MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_AFONSO:
            MessageBox(hWnd, _T("Nome:Afonso Henrique Pena Pedroso da Silva \n NºAluno:2021133861"), _T("Informação"), MB_OK | MB_ICONINFORMATION);
            break;
        }
        break;
    case WM_TIMER:
        InvalidateRect(hWnd, NULL, TRUE);  // Solicitar atualização da janela
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rect);
        if (atualizando) {
            DesenhaGraficoBarras(hdc, rect);
            TextOut(hdc, 10, rect.bottom - 30, nomeEmpresaMaisRecente, (int)_tcslen(nomeEmpresaMaisRecente));
            DesenhaLegenda(hdc, rect, td.empresas, *nEmpresasDisplay);
        }
        else {
            MostrarImagem(hdc, rect);
        }
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        td.continua = FALSE;
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Função de atualização das empresas
DWORD WINAPI atualizaTopEmpresas(LPVOID data) {
    TDATA* pData = (TDATA*)data;
    HANDLE hMap;
    Empresa* pEmpresas;
    int attempts = 0;

    while (pData->continua) {
        if (pData->pauseComando != 0) {
            Sleep(pData->pauseComando * 1000);
        }

        pData->pauseComando = 0;
        WaitForSingleObject(pData->hMutex, INFINITE);

        hMap = OpenFileMapping(FILE_MAP_READ, FALSE, _T("Empresa"));
        if (hMap != NULL) {
            pEmpresas = (Empresa*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(Empresa) * MAX_EMPRESAS);
            if (pEmpresas != NULL) {
                for (int i = 0; i < MAX_EMPRESAS; i++) {
                    td.empresas[i] = pEmpresas[i];
                }
                UnmapViewOfFile(pEmpresas);
                attempts = 0;
            }
            else {
                attempts++;
            }
            CloseHandle(hMap);
        }
        else {
            attempts++;
        }

        ReleaseMutex(pData->hMutex);

        // Simular uma empresa sendo transacionada
        if (td.empresas[0].nomeEmp[0] != '\0') {
            _tcscpy_s(nomeEmpresaMaisRecente, sizeof(nomeEmpresaMaisRecente) / sizeof(TCHAR), td.empresas[0].nomeEmp);
        }

        if (attempts >= 5) { // Se falhar 5 vezes consecutivas, parar a atualização
            atualizando = FALSE;
            break;
        }

        InvalidateRect(GetForegroundWindow(), NULL, TRUE);
        Sleep(2000);
    }

    return 0;
}

// Função para desenhar o gráfico de barras
void DesenhaGraficoBarras(HDC hdc, RECT rect) {
    int barWidth, barHeight, x, y;
    int maxHeight = rect.bottom - 100;
    int graphHeight = maxHeight - rect.top;
    int nEmpresas = min(*nEmpresasDisplay, MAX_EMPRESAS);
    TCHAR buffer[TAM_STR];
    TCHAR valorBuffer[TAM_STR];  // Buffer para os valores das ações

    barWidth = (rect.right - rect.left) / nEmpresas;

    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 255));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    for (int i = 0; i < nEmpresas; i++) {
        barHeight = (td.empresas[i].valorAcao - escalaInferior) * graphHeight / (escalaSuperior - escalaInferior);
        x = rect.left + i * barWidth;
        y = maxHeight - barHeight;

        // Desenhar a barra
        Rectangle(hdc, x, y, x + barWidth - 2, maxHeight);

        // Desenhar o nome da empresa
        _stprintf_s(buffer, _T("%s"), td.empresas[i].nomeEmp);
        TextOut(hdc, x, maxHeight + 5, buffer, (int)_tcslen(buffer));

        // Desenhar o valor da ação por cima da barra
        _stprintf_s(valorBuffer, _T("%lu"), td.empresas[i].valorAcao);
        TextOut(hdc, x, y - 20, valorBuffer, (int)_tcslen(valorBuffer));
    }

    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
}

BOOL isDWORD(TCHAR* str) {
    for (int i = 0; str[i] != _T('\0'); i++) {
        if (!_istdigit(str[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

// Função para mostrar a imagem BMP redimensionada
void MostrarImagem(HDC hdc, RECT rect) {
    // Caminho para a imagem BMP
    const wchar_t* imagePath = L"ISEC-LOGO-NOVO.bmp";

    // Carregar a imagem BMP
    HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, imagePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (hBitmap == NULL) {
        MessageBox(NULL, _T("Falha ao carregar a imagem"), _T("Erro"), MB_ICONERROR);
        return;
    }

    // Criar um DC de memória compatível
    HDC hMemDC = CreateCompatibleDC(hdc);
    SelectObject(hMemDC, hBitmap);

    // Obter as dimensões da imagem
    BITMAP bitmap;
    GetObject(hBitmap, sizeof(BITMAP), &bitmap);
    int imageWidth = bitmap.bmWidth;
    int imageHeight = bitmap.bmHeight;

    // Definir as novas dimensões da imagem (por exemplo, metade do tamanho original)
    int newWidth = imageWidth / 3;
    int newHeight = imageHeight / 3;

    // Calcular as dimensões e a posição para centralizar a imagem redimensionada
    int rectWidth = rect.right - rect.left;
    int rectHeight = rect.bottom - rect.top;
    int posX = rect.left + (rectWidth - newWidth) / 2;
    int posY = rect.top + (rectHeight - newHeight) / 2;

    // Redimensionar e desenhar a imagem
    StretchBlt(hdc, posX, posY, newWidth, newHeight, hMemDC, 0, 0, imageWidth, imageHeight, SRCCOPY);

    // Liberar os recursos
    DeleteDC(hMemDC);
    DeleteObject(hBitmap);
    Sleep(2000);
    PostQuitMessage(0);
}

void DesenhaLegenda(HDC hdc, RECT rect, Empresa* empresas, int nEmpresas) {
    const TCHAR* legendaX = _T("Empresas");
    const TCHAR* legendaY = _T("Valor da Ação");
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, rect.right / 2 - 30, rect.bottom - 20, legendaX, (int)_tcslen(legendaX));
    TextOut(hdc, 10, 10, legendaY, (int)_tcslen(legendaY));

    // Mostrando a última transação
    int offsetY = 30; // Offset from the top for each transaction

    RECT transacaoRectHeader = { rect.left, rect.top + offsetY - 20, rect.right, rect.top + offsetY };

    DrawText(hdc, _T("-------ULTIMA TRANSACAO--------"), -1, &transacaoRectHeader, DT_CENTER | DT_SINGLELINE);

    TCHAR ultimaTransacao[TAM_STR] = _T("");
    for (int i = 0; i < nEmpresas; i++) {
        if (_tcscmp(empresas[i].UltimaTransicao, _T(" ")) != 0) {
            _tcscpy_s(ultimaTransacao, TAM_STR, empresas[i].UltimaTransicao);
        }
    }

    RECT transacaoRect = { rect.left, rect.top + offsetY, rect.right, rect.top + offsetY + 20 };
    DrawText(hdc, ultimaTransacao, -1, &transacaoRect, DT_CENTER | DT_SINGLELINE);
}

// Recursos de interface do usuário em linha
const TCHAR szDialogTemplate[] =
_T("IDD_CONFIGURE DIALOGEX 0, 0, 260, 140\n")
_T("STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU\n")
_T("CAPTION \"Configure\"\n")
_T("FONT 8, \"MS Shell Dlg\"\n")
_T("BEGIN\n")
_T("    LTEXT           \"Min Scale:\",IDC_STATIC,10,10,50,10\n")
_T("    EDITTEXT        IDC_SCALE_MIN,70,10,50,14,ES_AUTOHSCROLL\n")
_T("    LTEXT           \"Max Scale:\",IDC_STATIC,10,30,50,10\n")
_T("    EDITTEXT        IDC_SCALE_MAX,70,30,50,14,ES_AUTOHSCROLL\n")
_T("    LTEXT           \"Number of Companies:\",IDC_STATIC,10,50,100,10\n")
_T("    EDITTEXT        IDC_NUM_EMPRESAS,110,50,50,14,ES_AUTOHSCROLL\n")
_T("    DEFPUSHBUTTON   \"OK\",IDOK,110,70,50,14\n")
_T("    PUSHBUTTON      \"Cancel\",IDCANCEL,170,70,50,14\n")
_T("END");

LRESULT CALLBACK InputWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndEdit;

    switch (uMsg) {
    case WM_CREATE:
        hwndEdit = CreateWindowEx(
            0, _T("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            10, 10, 260, 25,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        CreateWindow(
            _T("BUTTON"), _T("OK"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 50, 100, 30,
            hwnd, (HMENU)1, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        CreateWindow(
            _T("BUTTON"), _T("Cancel"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            120, 50, 100, 30,
            hwnd, (HMENU)2, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // OK button
            TCHAR text[256];
            GetWindowText(hwndEdit, text, 256);
            if (isDWORD(text)) {
                *nEmpresasDisplay = _tstoi(text);
            }
            else {
                MessageBox(hwnd, _T("Insira um número"), _T("Erro"), MB_OK | MB_ICONEXCLAMATION);
            }
            DestroyWindow(hwnd);
        }
        else if (LOWORD(wParam) == 2) { // Cancel button
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


DWORD DisplayConfirmSaveAsMessageBox() {
    const TCHAR CLASS_NAME[] = _T("InputWindowClass");

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = InputWindowProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        _T("Input"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 150,
        NULL, NULL, hInst, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 1;
}
