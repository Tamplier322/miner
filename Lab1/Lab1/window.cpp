#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <sstream>
#include <stack>
#include <gdiplus.h>
#include "window.h"
#include <fstream>
using namespace std;
#pragma comment (lib, "Gdiplus.lib")
using namespace Gdiplus;

#define ID_BUTTON_ENDGAME 1001
#define ID_BUTTON_NEWGAME 1002


enum Difficulty { EASY, MEDIUM, HARD };
Difficulty currentDifficulty = EASY;
int selectedDifficulty = 0;


HHOOK g_mouseHook = NULL;

HWND hWndEasy;     
HWND hWndMedium;
HWND hWndHard;

HINSTANCE hInst;
HWND hWndMain;
HWND hWndButtonEndGame;
HWND hWndButtonNewGame;
int cellSize = 30; // ������ ������
int numRows = 10;  // ���������� �����
int numCols = 10;  // ���������� ��������
bool** cellClicked = nullptr; // ������ ��� ������������, ���� �� ������ ������
bool** cellChecked = nullptr; // ������ ��� ������������, ���� �� ������ ���������
int** cellCount = nullptr; // ������ ��� ���������
int totalWhiteCells = 0; // ����� ���������� ����� ������
int score = 0; // ������� ����� ������
bool gameOver = false; // ����, ����������� �� ���������� ����
int numMines = 20; // ���������� ��� �� ����
int flagsPlaced = 0; // ���������� ������������� ������
bool** flags = nullptr; // ������ ��� ������������ ������������� ������
bool revealMinesAlways = true; // ���������� ��� �����������, ������ �� ���������� ������ ������
int remainingFlags = numMines; // ������� ���������� ������
UINT_PTR timerID = 1; // ������������� �������
const UINT timerInterval = 100; // �������� ������� � ������������� (100 �����������)
HBITMAP hFlagBitmap = nullptr;
bool isExitButtonHovered = false;
bool isNewGameButtonHovered = false;

// ���������� ��������� ������ ����� �� ����
void GenerateRandomBoard(int numBlackPoints) {
    srand(static_cast<unsigned int>(time(nullptr))); // �������������� ��������� ��������� ����� ������� ��������

    // ��������� ���� ���������� ������� ������� � ������������ ����� ������
    totalWhiteCells = numRows * numCols - numBlackPoints;

    // �������������� �������
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            cellClicked[i][j] = false;
            cellChecked[i][j] = false;
            cellCount[i][j] = 0;
        }
    }

    while (numBlackPoints > 0) {
        int x = rand() % numCols;
        int y = rand() % numRows;
        if (!cellClicked[y][x]) {
            cellClicked[y][x] = true;
            numBlackPoints--;
        }
    }

    // ��������� �������� ��� ����� ������ �� ������ ��� � ������� 1 �� ������
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            if (!cellClicked[i][j]) {
                // ��������� ������ �������� ������
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        int nx = j + dx;
                        int ny = i + dy;
                        if (nx >= 0 && nx < numCols && ny >= 0 && ny < numRows && cellClicked[ny][nx]) {
                            cellCount[i][j]++;
                        }
                    }   
                }
            }
        }
    }

    // �������������� ������ ��� ������������ ������������� ������
    flags = new bool* [numRows];
    for (int i = 0; i < numRows; i++) {
        flags[i] = new bool[numCols]();
    }
}

// ������� ��������� �������� ����
void DrawBoard(HDC hdc) {
    for (int i = 0; i <= numRows; i++) {
        int y = i * cellSize;
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, numCols * cellSize, y);
    }
    for (int i = 0; i <= numCols; i++) {
        int x = i * cellSize;
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, numRows * cellSize);
    }


    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            RECT cellRect = { j * cellSize, i * cellSize, (j + 1) * cellSize, (i + 1) * cellSize };

            if (gameOver || revealMinesAlways) {
                if (cellClicked[i][j]) {
                    HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
                    FillRect(hdc, &cellRect, hBlackBrush);
                    DeleteObject(hBlackBrush);
                }
            }

            if (!cellClicked[i][j] && cellChecked[i][j]) {
                if (cellCount[i][j] > 0) {
                    std::wstringstream ss;
                    ss << cellCount[i][j];
                    TextOut(hdc, cellRect.left + 4, cellRect.top + 4, ss.str().c_str(), static_cast<int>(ss.str().length()));
                }
                else {
                    // ������������ "0" ��� ����� ������ ��� ������ �������
                    TextOut(hdc, cellRect.left + 4, cellRect.top + 4, L"0", 1);
                }
            }
            else if (flags[i][j]) {
                HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 0, 0));
                HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hRedBrush));

                POINT points[3];
                points[0] = { cellRect.left, cellRect.top };
                points[1] = { cellRect.right, cellRect.top };
                points[2] = { (cellRect.left + cellRect.right) / 2, cellRect.bottom };
                Polygon(hdc, points, 3);

                SelectObject(hdc, hOldBrush);
                DeleteObject(hRedBrush);
            }
        }

    }

    // ������������ ������� ����� ������
    std::wstringstream ss;
    ss << L"����� ������: " << score;
    TextOut(hdc, 610, 90, ss.str().c_str(), static_cast<int>(ss.str().length()));

    // ������������ ������� ���������� ������
    std::wstringstream flagsCounter;
    flagsCounter << L"���������� �����: " << remainingFlags;
    TextOut(hdc, 760, 90, flagsCounter.str().c_str(), static_cast<int>(flagsCounter.str().length()));

}

// ������� �������� ������� ������ � ����
bool CheckWin() {
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            if (cellClicked[i][j] && cellCount[i][j] != -1 && !flags[i][j]) {
                return false; // ���� ���� �������� ��-���� ��� �����, ������������ �� �������
            }
        }
    }
    return true; // ���� ��� �������� ������ - ���� ��� � �������, ������������ �������
}

// ������� �������� ������� ������ � ����� �����
void CheckForWin() {
    if (!gameOver) {
        bool allFlagsOnMines = true; // �������������� ���������� ��� ��������

        for (int i = 0; i < numRows; i++) {
            for (int j = 0; j < numCols; j++) {
                if (cellClicked[i][j] && cellCount[i][j] != -1 && !flags[i][j]) {
                    return; // ���� ���� �������� ��-���� ��� �����, �������
                }
                if (!cellClicked[i][j] && !cellChecked[i][j]) {
                    allFlagsOnMines = false; // ���� ���� ������������� ������, ���������� ����
                }
            }
        }

        if (allFlagsOnMines) { // ���� ��� ����� ��������� �� �����
            MessageBox(hWndMain, _T("�����������! �� ��������!"), _T("������"), MB_OK | MB_ICONINFORMATION);
            gameOver = true; 
            //������������� ������
            EnableWindow(hWndButtonEndGame, TRUE); 
            EnableWindow(hWndButtonNewGame, TRUE); 
        }
    }
}

bool IsInImageArea(POINT pt) {
    int imageX = 650;
    int imageY = 150;
    int imageWidth = 180;
    int imageHeight = 180;

    // ���������, �������� �� ����� � ������� �����������
    if (pt.x >= imageX && pt.x <= imageX + imageWidth && pt.y >= imageY && pt.y <= imageY + imageHeight) {
        return true;
    }

    return false;
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MOUSEHOOKSTRUCT* pMouseStruct = (MOUSEHOOKSTRUCT*)lParam;

        if (pMouseStruct != NULL && wParam == WM_LBUTTONDOWN) {
            // ���������, ��� �� ���� �������� � ������� �����������
            if (IsInImageArea(pMouseStruct->pt)) {
                // ���� ���� ��� � ������� �����������, �������� ����� � ����
                MessageBox(NULL, _T("������� �� �����������!"), _T("����������"), MB_OK | MB_ICONINFORMATION);
            }
        }
    }

    // �������� ���������� ���������� ����������� � �������
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

bool SetMouseHook() {
    g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
    return g_mouseHook != NULL;
}

// ������� ��� �������� ����������� ����
void UnhookMouse() {
    if (g_mouseHook != NULL) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = NULL;
    }
}

// ������� ����������� ���� ��� �� ������� ���� ��� ���������� ����
void RevealAllMines(HDC hdc) {
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            if (cellClicked[i][j]) {
                HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
                HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBlackBrush));

                RECT cellRect = { j * cellSize, i * cellSize, (j + 1) * cellSize, (i + 1) * cellSize };
                Ellipse(hdc, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom);

                SelectObject(hdc, hOldBrush);
                DeleteObject(hBlackBrush);
            }
        }
    }
}

// ������� ����������� ���� ��� �� ������� ���� ��� ���������� ����
void SpreadZeros(int x, int y) {
    std::stack<std::pair<int, int>> zeroStack;
    zeroStack.push(std::make_pair(x, y));

    while (!zeroStack.empty()) {
        std::pair<int, int> current = zeroStack.top();
        zeroStack.pop();
        int cx = current.first;
        int cy = current.second;

        // ���������, ��� ������� ������� ����� ������ ����� 0
        if (cellCount[cy][cx] == 0) {
            // ���������� "0" ��� ������� ����� ������
            cellChecked[cy][cx] = true;
            score++;
            // �������������� ����
            InvalidateRect(hWndMain, NULL, TRUE);

            // ���������� ������ �������� ������
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < numCols && ny >= 0 && ny < numRows) {
                        if (!cellChecked[ny][nx]) {
                            // ��������� �������� ������ � ������� ��������� � ����
                            zeroStack.push(std::make_pair(nx, ny));
                        }
                    }
                }
            }
        }
    }
}

void ResetGame() {
    // ����������� ������ ��� �������� cellClicked, cellChecked, cellCount � flags
    if (cellClicked != nullptr) {
        delete[] cellClicked;
        delete[] cellChecked;
        delete[] cellCount;
    }

    if (flags != nullptr) {
        delete[] flags;
    }

    cellClicked = new bool* [numRows];
    cellChecked = new bool* [numRows];
    cellCount = new int* [numRows];
    for (int i = 0; i < numRows; i++) {
        cellClicked[i] = new bool[numCols]();
        cellChecked[i] = new bool[numCols]();
        cellCount[i] = new int[numCols]();
    }

    GenerateRandomBoard(numMines); // ���������� ��������� ������ �����
    score = 0; // ���������� ������� ����� ������

    // �������������� ������ flags ������
    flags = new bool* [numRows];
    for (int i = 0; i < numRows; i++) {
        flags[i] = new bool[numCols]();
    }
    flagsPlaced = 0;
    remainingFlags = numMines;
    // �������������� ����
    InvalidateRect(hWndMain, NULL, TRUE);
}


// ������� ��������� ��������� ����
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    switch (message) {
    case WM_CREATE:
        break;
    case WM_TIMER:
        // ��������� ������ ������ 100 �����������
        CheckForWin();
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // ������������ ������� ����
        DrawBoard(hdc);

        // ������� ������ Graphics � ��������� �����������
        Graphics graphics(hdc);
        Bitmap bmp(L"C:\\plusi\\img\\mine.png");

        int x = 610; // X-����������
        int y = 150; // Y-����������
        int width = bmp.GetWidth(); // ������ �����������
        int height = bmp.GetHeight(); // ������ �����������

        // ������� ����������� �� �����
        graphics.DrawImage(&bmp, x, y, width, height);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        // ����������� ������ ��� �������� cellClicked, cellChecked � cellCount
        for (int i = 0; i < numRows; i++) {
            delete[] cellClicked[i];
            delete[] cellChecked[i];
            delete[] cellCount[i];
        }
        delete[] cellClicked;
        delete[] cellChecked;
        delete[] cellCount;

        KillTimer(hWnd, timerID);
        PostQuitMessage(0);
        GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BUTTON_ENDGAME:
            // ��������� ������� ������ "��������� ����"
            PostQuitMessage(0);
            break;
        case ID_BUTTON_NEWGAME:
            // ��������� ������� ������ "����� ����"
            ResetGame();
            gameOver = false;
            break;
        case ID_DIFFICULTY_EASY:
            CheckRadioButton(hWndMain, ID_DIFFICULTY_EASY, ID_DIFFICULTY_HARD, ID_DIFFICULTY_EASY);
            numRows = 10;
            numCols = 10;
            numMines = 20;
            ResetGame();
            break;
        case ID_DIFFICULTY_MEDIUM:
            CheckRadioButton(hWndMain, ID_DIFFICULTY_EASY, ID_DIFFICULTY_HARD, ID_DIFFICULTY_MEDIUM);
            numRows = 15;
            numCols = 15;
            numMines = 35;
            ResetGame();
            break;
        case ID_DIFFICULTY_HARD:
            CheckRadioButton(hWndMain, ID_DIFFICULTY_EASY, ID_DIFFICULTY_HARD, ID_DIFFICULTY_HARD);
            numRows = 20;
            numCols = 20;
            numMines = 50;
            ResetGame();
            break;
        }
        break;
    case WM_LBUTTONDOWN:
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (IsInImageArea(pt)) {
            MessageBox(hWnd, L"������� �� �����������!", L"����������", MB_OK | MB_ICONINFORMATION);
        }
        if (!gameOver) {
            int x = LOWORD(lParam) / cellSize;
            int y = HIWORD(lParam) / cellSize;

            // ���������, ��� ������� ��������� � �������� �������� ����
            if (x >= 0 && x < numCols && y >= 0 && y < numRows) {
                if (cellClicked[y][x]) {
                    gameOver = true;
                    EnableWindow(hWndButtonEndGame, TRUE);
                    EnableWindow(hWndButtonNewGame, TRUE);

                    HDC hdc = GetDC(hWnd);
                    RevealAllMines(hdc);
                    ReleaseDC(hWnd, hdc);

                    MessageBox(hWnd, _T("Game Over"), _T("���������"), MB_OK | MB_ICONERROR);
                }
                else if (!cellChecked[y][x]) {
                    if (cellCount[y][x] == 0) {
                        // ��������������� �����
                        SpreadZeros(x, y);
                    }
                    else {
                        // ���������� ������� ��� ����� ������
                        cellChecked[y][x] = true;
                        score++;
                        // �������������� ����
                        HDC hdc = GetDC(hWnd);
                        DrawBoard(hdc); // �������������� ����
                        ReleaseDC(hWnd, hdc);
                    }
                }
            }
        }
        break;
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
        HDC hdcButton = lpDIS->hDC;

        if (lpDIS->CtlID == ID_BUTTON_ENDGAME) {
            HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 0, 0)); // �������� ���� ����
            FillRect(hdcButton, &lpDIS->rcItem, hRedBrush); // ����������� ��� ������� ������

            HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Arial"));
            HFONT hOldFont = (HFONT)SelectObject(hdcButton, hFont);

            SetTextColor(hdcButton, RGB(200, 162, 200)); // ������������� ���� ������
            SetBkMode(hdcButton, TRANSPARENT); // ������������� ���������� ��� ������
            RECT textRect = lpDIS->rcItem;
            DrawText(hdcButton, _T("��������� ����"), -1, &textRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            // ����������� ��������� �����
            DeleteObject(hRedBrush);
        }
        else if (lpDIS->CtlID == ID_BUTTON_NEWGAME) {
            HBRUSH hBlueBrush = CreateSolidBrush(RGB(0, 0, 255)); // �������� ���� ����
            FillRect(hdcButton, &lpDIS->rcItem, hBlueBrush); // ����������� ��� ����� ������

            HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Arial"));
            HFONT hOldFont = (HFONT)SelectObject(hdcButton, hFont);

            SetTextColor(hdcButton, RGB(200, 162, 200)); // ������������� ���� ������
            SetBkMode(hdcButton, TRANSPARENT); // ������������� ���������� ��� ������
            RECT textRect = lpDIS->rcItem;
            DrawText(hdcButton, _T("����� ����"), -1, &textRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            DeleteObject(hBlueBrush);
        }

        return TRUE;
    }
    case WM_RBUTTONDOWN:
        if (!gameOver) {
            int x = LOWORD(lParam) / cellSize;
            int y = HIWORD(lParam) / cellSize;

            // ���������, ��� ������� ��������� � �������� �������� ����
            if (x >= 0 && x < numCols && y >= 0 && y < numRows) {
                // ���� ���� ��� ����������, ������� ��� � ����������� ������� ���������� ������
                if (!cellChecked[y][x]) {
                    if (flags[y][x]) {
                        flags[y][x] = false;
                        flagsPlaced--;
                        remainingFlags++;
                    }
                    else if (remainingFlags > 0) {
                        flags[y][x] = true;
                        flagsPlaced++;
                        remainingFlags--;
                    }

                    // �������������� ����
                    InvalidateRect(hWnd, NULL, TRUE);
                }
            }
             // ���������, ������� �� ����� ����� ������� ����
            if (flagsPlaced == numMines && CheckWin()) {
                MessageBox(hWnd, _T("�����������! �� ��������!"), _T("������"), MB_OK | MB_ICONINFORMATION);
                gameOver = true;
                EnableWindow(hWndButtonEndGame, TRUE);
                EnableWindow(hWndButtonNewGame, TRUE);
            }
        }
        break;
    
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

// ������� �������� � ������������� ����
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    // �������������� ������� ��� ������������ ������� �����, ����������� ����� � ���������
    cellClicked = new bool* [numRows];
    cellChecked = new bool* [numRows];
    cellCount = new int* [numRows];
    for (int i = 0; i < numRows; i++) {
        cellClicked[i] = new bool[numCols]();
        cellChecked[i] = new bool[numCols]();
        cellCount[i] = new int[numCols]();
    }


    GenerateRandomBoard(numMines); // ���������� ��������� ������ �����

    timerID = SetTimer(hWndMain, 1, timerInterval, NULL);
    if (timerID == 0) {
        MessageBox(hWndMain, _T("�� ������� ������� ������."), _T("������"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hWndMain = CreateWindow(
        _T("SapperApp"), _T("�����"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(numCols * cellSize * 1.5) + 500, numRows * cellSize + 600, NULL, NULL, hInstance, NULL);


    hWndButtonEndGame = CreateWindow(
        _T("BUTTON"), _T("��������� ����"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        610, 10, 150, 40, hWndMain, (HMENU)ID_BUTTON_ENDGAME, hInstance, NULL);

    hWndButtonNewGame = CreateWindow(
        _T("BUTTON"), _T("����� ����"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        770, 10, 150, 40, hWndMain, (HMENU)ID_BUTTON_NEWGAME, hInstance, NULL);

    HWND hWndEasy = CreateWindow(
        _T("BUTTON"), _T("�����"), WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
        610, 60, 80, 20, hWndMain, (HMENU)ID_DIFFICULTY_EASY, hInstance, NULL);

    HWND hWndMedium = CreateWindow(
        _T("BUTTON"), _T("������"), WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
        700, 60, 80, 20, hWndMain, (HMENU)ID_DIFFICULTY_MEDIUM, hInstance, NULL);

    HWND hWndHard = CreateWindow(
        _T("BUTTON"), _T("������"), WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
        790, 60, 80, 20, hWndMain, (HMENU)ID_DIFFICULTY_HARD, hInstance, NULL);

    if (!hWndMain) {
        return FALSE;
    }

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    return TRUE;
}

// ����� ����� � ����������
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // ������������ ����� ����
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("SapperApp");
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) {
        return FALSE;
    }

    // �������������� ������� ���� ����������
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // �������� ���� ��������� ���������
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
