#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <sstream>
#include <stack>

#define ID_BUTTON_ENDGAME 1001
#define ID_BUTTON_NEWGAME 1002

// Глобальные переменные
HINSTANCE hInst;
HWND hWndMain;
HWND hWndButtonEndGame;
HWND hWndButtonNewGame;
int cellSize = 30; // Размер ячейки
int numRows = 10;  // Количество строк
int numCols = 10;  // Количество столбцов
bool** cellClicked = nullptr; // Массив для отслеживания, была ли ячейка нажата
bool** cellChecked = nullptr; // Массив для отслеживания, была ли ячейка проверена
int** cellCount = nullptr; // Массив для счетчиков
int totalWhiteCells = 0; // Общее количество белых клеток
int score = 0; // Счетчик белых клеток
bool gameOver = false; // Флаг, указывающий на завершение игры
int numMines = 20; // Количество мин на поле
int flagsPlaced = 0; // Количество установленных флагов
bool** flags = nullptr; // Массив для отслеживания установленных флагов
bool revealMinesAlways = false; // Переменная для определения, всегда ли отображать черные клетки
int remainingFlags = numMines; // Счетчик оставшихся флагов
UINT_PTR timerID = 1; // Идентификатор таймера
const UINT timerInterval = 100; // Интервал таймера в миллисекундах (100 миллисекунд)


// Генерирует случайные черные точки на поле
void GenerateRandomBoard() {
    srand(static_cast<unsigned int>(time(nullptr))); // Инициализируем генератор случайных чисел текущим временем

    // Заполняем поле случайными черными точками и подсчитываем белые клетки
    int numBlackPoints = 20; // Задаем количество черных точек
    totalWhiteCells = numRows * numCols - numBlackPoints;

    // Инициализируем массивы
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

    // Вычисляем счетчики для белых клеток на основе мин в радиусе 1 от клетки
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            if (!cellClicked[i][j]) {
                // Проверяем восемь соседних клеток
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

    // Инициализируем массив для отслеживания установленных флагов
    flags = new bool* [numRows];
    for (int i = 0; i < numRows; i++) {
        flags[i] = new bool[numCols]();
    }
}

// Функция отрисовки игрового поля
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
                    // Отрисовываем "0" для белых клеток без черных соседей
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

    // Отрисовываем счетчик белых клеток
    std::wstringstream ss;
    ss << L"Белых клеток: " << score;
    TextOut(hdc, 10, numRows * cellSize + 10, ss.str().c_str(), static_cast<int>(ss.str().length()));

    // Отрисовываем счетчик оставшихся флагов
    std::wstringstream flagsCounter;
    flagsCounter << L"Оставшиеся флаги: " << remainingFlags;
    TextOut(hdc, 200, numRows * cellSize + 10, flagsCounter.str().c_str(), static_cast<int>(flagsCounter.str().length()));

}

// Функция проверки условия победы в игре
bool CheckWin() {
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            if (cellClicked[i][j] && cellCount[i][j] != -1 && !flags[i][j]) {
                return false; // Если есть открытая не-мина без флага, пользователь не выиграл
            }
        }
    }
    return true; // Если все открытые ячейки - мины или с флагами, пользователь выиграл
}

// Функция проверки условия победы и вывод этого
void CheckForWin() {
    if (!gameOver) {
        bool allFlagsOnMines = true; // Дополнительная переменная для проверки

        for (int i = 0; i < numRows; i++) {
            for (int j = 0; j < numCols; j++) {
                if (cellClicked[i][j] && cellCount[i][j] != -1 && !flags[i][j]) {
                    return; // Если есть открытая не-мина без флага, выходим
                }
                if (!cellClicked[i][j] && !cellChecked[i][j]) {
                    allFlagsOnMines = false; // Если есть непроверенная клетка, выставляем флаг
                }
            }
        }

        if (allFlagsOnMines) { // Если все флаги находятся на минах
            MessageBox(hWndMain, _T("Поздравляем! Вы выиграли!"), _T("Победа"), MB_OK | MB_ICONINFORMATION);
            gameOver = true; 
            //Разблокировка кнопок
            EnableWindow(hWndButtonEndGame, TRUE); 
            EnableWindow(hWndButtonNewGame, TRUE); 
        }
    }
}

// Функция отображения всех мин на игровом поле при завершении игры
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

// Функция отображения всех мин на игровом поле при завершении игры
void SpreadZeros(int x, int y) {
    std::stack<std::pair<int, int>> zeroStack;
    zeroStack.push(std::make_pair(x, y));

    while (!zeroStack.empty()) {
        std::pair<int, int> current = zeroStack.top();
        zeroStack.pop();
        int cx = current.first;
        int cy = current.second;

        // Проверяем, что счетчик текущей белой клетки равен 0
        if (cellCount[cy][cx] == 0) {
            // Отображаем "0" для текущей белой клетки
            cellChecked[cy][cx] = true;
            score++;
            // Перерисовываем окно
            InvalidateRect(hWndMain, NULL, TRUE);

            // Перебираем восемь соседних клеток
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < numCols && ny >= 0 && ny < numRows) {
                        if (!cellChecked[ny][nx]) {
                            // Добавляем соседнюю клетку с нулевым счетчиком в стек
                            zeroStack.push(std::make_pair(nx, ny));
                        }
                    }
                }
            }
        }
    }
}


void ResetGame() {
    // Освобождаем память для массивов cellClicked, cellChecked и cellCount
    for (int i = 0; i < numRows; i++) {
        delete[] cellClicked[i];
        delete[] cellChecked[i];
        delete[] cellCount[i];
    }
    delete[] cellClicked;
    delete[] cellChecked;
    delete[] cellCount;

    // Инициализируем новую игру
    cellClicked = new bool* [numRows];
    cellChecked = new bool* [numRows];
    cellCount = new int* [numRows];
    for (int i = 0; i < numRows; i++) {
        cellClicked[i] = new bool[numCols]();
        cellChecked[i] = new bool[numCols]();
        cellCount[i] = new int[numCols]();
    }

    GenerateRandomBoard(); // Генерируем случайные черные точки
    score = 0; // Сбрасываем счетчик белых клеток

    // Освобождаем память для массива flags и инициализируем его заново
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
    // Перерисовываем окно
    InvalidateRect(hWndMain, NULL, TRUE);
}



// Функция обработки сообщений окна
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TIMER:
        // Проверяем победу каждые 100 миллисекунд
        CheckForWin();
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Отрисовываем игровое поле
        DrawBoard(hdc);

        EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
        // Освобождаем память для массивов cellClicked, cellChecked и cellCount
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
            // Обработка нажатия кнопки "Завершить игру"
            PostQuitMessage(0);
            break;
        case ID_BUTTON_NEWGAME:
            // Обработка нажатия кнопки "Новая игра"
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

            // Проверяем, что нажатие произошло в пределах игрового поля
            if (x >= 0 && x < numCols && y >= 0 && y < numRows) {
                if (cellClicked[y][x]) {
                    gameOver = true;
                    EnableWindow(hWndButtonEndGame, TRUE);
                    EnableWindow(hWndButtonNewGame, TRUE);

                    HDC hdc = GetDC(hWnd);
                    RevealAllMines(hdc);
                    ReleaseDC(hWnd, hdc);

                    MessageBox(hWnd, _T("Game Over"), _T("Сообщение"), MB_OK | MB_ICONERROR);
                }
                else if (!cellChecked[y][x]) {
                    if (cellCount[y][x] == 0) {
                        // Распространение нулей
                        SpreadZeros(x, y);
                    }
                    else {
                        // Отображаем счетчик для белых клеток
                        cellChecked[y][x] = true;
                        score++;
                        // Перерисовываем окно
                        HDC hdc = GetDC(hWnd);
                        DrawBoard(hdc); // Перерисовываем поле
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

            // Проверяем, что нажатие произошло в пределах игрового поля
            if (x >= 0 && x < numCols && y >= 0 && y < numRows) {
                // Если флаг уже установлен, убираем его и увеличиваем счетчик оставшихся флагов
                if (flags[y][x]) {
                    flags[y][x] = false;
                    flagsPlaced--;
                    remainingFlags++;
                }
                // Иначе устанавливаем флаг и уменьшаем счетчик оставшихся флагов
                else if (flagsPlaced < numMines && remainingFlags > 0) {
                    flags[y][x] = true;
                    flagsPlaced++;
                    remainingFlags--;
                }

                // Перерисовываем окно
                InvalidateRect(hWnd, NULL, TRUE);

                // Проверяем, выиграл ли игрок после каждого хода
                if (flagsPlaced == numMines && CheckWin()) {
                    MessageBox(hWnd, _T("Поздравляем! Вы выиграли!"), _T("Победа"), MB_OK | MB_ICONINFORMATION);
                    gameOver = true; // Устанавливаем флаг "Game Over"
                    EnableWindow(hWndButtonEndGame, TRUE); // Кнопка "Завершить игру" доступна
                    EnableWindow(hWndButtonNewGame, TRUE); // Кнопка "Новая игра" доступна
                }
            }
        }
        break;


    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Функция создания и инициализации окна
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    // Инициализируем массивы для отслеживания нажатых ячеек, проверенных ячеек и счетчиков
    cellClicked = new bool* [numRows];
    cellChecked = new bool* [numRows];
    cellCount = new int* [numRows];
    for (int i = 0; i < numRows; i++) {
        cellClicked[i] = new bool[numCols]();
        cellChecked[i] = new bool[numCols]();
        cellCount[i] = new int[numCols]();
    }

    GenerateRandomBoard(); // Генерируем случайные черные точки

    timerID = SetTimer(hWndMain, 1, timerInterval, NULL);
    if (timerID == 0) {
        MessageBox(hWndMain, _T("Не удалось создать таймер."), _T("Ошибка"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hWndMain = CreateWindow(
        _T("SapperApp"), _T("Сапер"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(numCols * cellSize * 1.5) + 20, numRows * cellSize + 120, NULL, NULL, hInstance, NULL);


    hWndButtonEndGame = CreateWindow(
        _T("BUTTON"), _T("Завершить игру"), WS_CHILD | WS_VISIBLE,
        10, numRows * cellSize + 40, 150, 40, hWndMain, (HMENU)ID_BUTTON_ENDGAME, hInstance, NULL);

    hWndButtonNewGame = CreateWindow(
        _T("BUTTON"), _T("Новая игра"), WS_CHILD | WS_VISIBLE,
        170, numRows * cellSize + 40, 150, 40, hWndMain, (HMENU)ID_BUTTON_NEWGAME, hInstance, NULL);


    if (!hWndMain) {
        return FALSE;
    }

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    return TRUE;
}

// Точка входа в приложение
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Регистрируем класс окна
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

    // Инициализируем главное окно приложения
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    // Основной цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
