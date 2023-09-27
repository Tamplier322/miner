#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <sstream>
#include <stack>

#define ID_BUTTON_ENDGAME 1001
#define ID_BUTTON_NEWGAME 1002

// ���������� ����������
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
bool revealMinesAlways = false; // ���������� ��� �����������, ������ �� ���������� ������ ������
int remainingFlags = numMines; // ������� ���������� ������
UINT_PTR timerID = 1; // ������������� �������
const UINT timerInterval = 100; // �������� ������� � ������������� (100 �����������)


// ���������� ��������� ������ ����� �� ����
void GenerateRandomBoard() {
    srand(static_cast<unsigned int>(time(nullptr))); // �������������� ��������� ��������� ����� ������� ��������

    // ��������� ���� ���������� ������� ������� � ������������ ����� ������
    int numBlackPoints = 20; // ������ ���������� ������ �����
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
    TextOut(hdc, 10, numRows * cellSize + 10, ss.str().c_str(), static_cast<int>(ss.str().length()));

    // ������������ ������� ���������� ������
    std::wstringstream flagsCounter;
    flagsCounter << L"���������� �����: " << remainingFlags;
    TextOut(hdc, 200, numRows * cellSize + 10, flagsCounter.str().c_str(), static_cast<int>(flagsCounter.str().length()));

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
    // ����������� ������ ��� �������� cellClicked, cellChecked � cellCount
    for (int i = 0; i < numRows; i++) {
        delete[] cellClicked[i];
        delete[] cellChecked[i];
        delete[] cellCount[i];
    }
    delete[] cellClicked;
    delete[] cellChecked;
    delete[] cellCount;

    // �������������� ����� ����
    cellClicked = new bool* [numRows];
    cellChecked = new bool* [numRows];
    cellCount = new int* [numRows];
    for (int i = 0; i < numRows; i++) {
        cellClicked[i] = new bool[numCols]();
        cellChecked[i] = new bool[numCols]();
        cellCount[i] = new int[numCols]();
    }

    GenerateRandomBoard(); // ���������� ��������� ������ �����
    score = 0; // ���������� ������� ����� ������

    // ����������� ������ ��� ������� flags � �������������� ��� ������
    for (int i = 0; i < numRows; i++) {
        delete[] flags[i];
    }
    delete[] flags;

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
    switch (message) {
    case WM_TIMER:
        // ��������� ������ ������ 100 �����������
        CheckForWin();
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // ������������ ������� ����
        DrawBoard(hdc);

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
            EnableWindow(hWndButtonEndGame, FALSE); 
            EnableWindow(hWndButtonNewGame, FALSE); 
            break;
        }
        break;
    case WM_CREATE:
        EnableWindow(hWndButtonEndGame, FALSE);
        EnableWindow(hWndButtonNewGame, FALSE);
        break;
    case WM_LBUTTONDOWN:
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

    case WM_RBUTTONDOWN:
        if (!gameOver) {
            int x = LOWORD(lParam) / cellSize;
            int y = HIWORD(lParam) / cellSize;

            // ���������, ��� ������� ��������� � �������� �������� ����
            if (x >= 0 && x < numCols && y >= 0 && y < numRows) {
                // ���� ���� ��� ����������, ������� ��� � ����������� ������� ���������� ������
                if (flags[y][x]) {
                    flags[y][x] = false;
                    flagsPlaced--;
                    remainingFlags++;
                }
                // ����� ������������� ���� � ��������� ������� ���������� ������
                else if (flagsPlaced < numMines && remainingFlags > 0) {
                    flags[y][x] = true;
                    flagsPlaced++;
                    remainingFlags--;
                }

                // �������������� ����
                InvalidateRect(hWnd, NULL, TRUE);

                // ���������, ������� �� ����� ����� ������� ����
                if (flagsPlaced == numMines && CheckWin()) {
                    MessageBox(hWnd, _T("�����������! �� ��������!"), _T("������"), MB_OK | MB_ICONINFORMATION);
                    gameOver = true; // ������������� ���� "Game Over"
                    EnableWindow(hWndButtonEndGame, TRUE); // ������ "��������� ����" ��������
                    EnableWindow(hWndButtonNewGame, TRUE); // ������ "����� ����" ��������
                }
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

    GenerateRandomBoard(); // ���������� ��������� ������ �����

    timerID = SetTimer(hWndMain, 1, timerInterval, NULL);
    if (timerID == 0) {
        MessageBox(hWndMain, _T("�� ������� ������� ������."), _T("������"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hWndMain = CreateWindow(
        _T("SapperApp"), _T("�����"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(numCols * cellSize * 1.5) + 20, numRows * cellSize + 120, NULL, NULL, hInstance, NULL);


    hWndButtonEndGame = CreateWindow(
        _T("BUTTON"), _T("��������� ����"), WS_CHILD | WS_VISIBLE,
        10, numRows * cellSize + 40, 150, 40, hWndMain, (HMENU)ID_BUTTON_ENDGAME, hInstance, NULL);

    hWndButtonNewGame = CreateWindow(
        _T("BUTTON"), _T("����� ����"), WS_CHILD | WS_VISIBLE,
        170, numRows * cellSize + 40, 150, 40, hWndMain, (HMENU)ID_BUTTON_NEWGAME, hInstance, NULL);


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

    // �������� ���� ��������� ���������
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
