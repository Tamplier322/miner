#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <stack>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

LPCTSTR RegTypeToString(DWORD regType) {
    switch (regType) {
    case REG_SZ:
        return _T("REG_SZ");
    case REG_EXPAND_SZ:
        return _T("REG_EXPAND_SZ");
    case REG_BINARY:
        return _T("REG_BINARY");
    case REG_DWORD:
        return _T("REG_DWORD");
    case REG_QWORD:
        return _T("REG_QWORD");
    case REG_MULTI_SZ:
        return _T("REG_MULTI_SZ");
    case REG_NONE:
        return _T("REG_NONE");
        // Другие типы реестра могут быть добавлены по мере необходимости
    default:
        return _T("Неизвестный тип");
    }
}

// Глобальные переменные для текущего ключа и стека папок
HKEY hCurrentKey = HKEY_LOCAL_MACHINE;
std::stack<std::wstring> folderStack;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Регистрация класса окна
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = _T("RegistryViewer");
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) {
        MessageBox(nullptr, _T("Не удалось зарегистрировать класс окна!"), _T("Ошибка"), MB_ICONERROR);
        return 1;
    }

    // Создание главного окна
    HWND hWnd = CreateWindow(_T("RegistryViewer"), _T("Просмотр реестра Windows"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        MessageBox(nullptr, _T("Не удалось создать окно!"), _T("Ошибка"), MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Основной цикл сообщений
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}


void UpdateFolderList(HKEY hKey, std::vector<std::wstring>& subkeys, HWND hListBox, HWND hStatusLabel) {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    subkeys.clear();

    DWORD subkeyCount;
    RegQueryInfoKey(hKey, nullptr, nullptr, nullptr, &subkeyCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    // Отображаем папки
    for (DWORD i = 0; i < subkeyCount; i++) {
        TCHAR subkeyName[255];
        DWORD subkeyNameLen = 255;
        if (RegEnumKeyEx(hKey, i, subkeyName, &subkeyNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            subkeys.push_back(subkeyName);
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)subkeyName);
        }
    }

    // Отображаем значения
    DWORD valueCount;
    RegQueryInfoKey(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &valueCount, nullptr, nullptr, nullptr, nullptr);

    for (DWORD i = 0; i < valueCount; i++) {
        TCHAR valueName[255];
        DWORD valueNameLen = 255;
        DWORD valueType;
        BYTE valueData[4]; // Для четырех двузначных чисел
        DWORD valueDataSize = sizeof(valueData);

        if (RegEnumValue(hKey, i, valueName, &valueNameLen, nullptr, &valueType, valueData, &valueDataSize) == ERROR_SUCCESS) {
            TCHAR valueInfo[255];

            if (valueType == REG_DWORD && valueDataSize == sizeof(DWORD)) {
                DWORD* valueDataDword = reinterpret_cast<DWORD*>(valueData);
                _stprintf_s(valueInfo, _T("%s %02d %02d %02d %02d"), valueName, (*valueDataDword & 0xFF), ((*valueDataDword >> 8) & 0xFF), ((*valueDataDword >> 16) & 0xFF), ((*valueDataDword >> 24) & 0xFF));
            }
            else {
                _stprintf_s(valueInfo, _T("%s (%s)"), valueName, RegTypeToString(valueType));
            }

            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)valueInfo);
        }
    }

    if (subkeyCount == 0 && valueCount == 0) {
        SetWindowText(hStatusLabel, _T("Папка пуста"));
    }
    else {
        SetWindowText(hStatusLabel, _T(""));
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hListBox;
    static HWND hEdit;
    static HWND hButtonGo;
    static HWND hButtonBack;
    static HWND hStatusLabel;
    static std::vector<std::wstring> subkeys;

    switch (message) {
    case WM_CREATE: {
        // Создаем список (List Box)
        hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, _T("LISTBOX"), nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL,
            10, 50, 700, 500, hWnd, (HMENU)100, GetModuleHandle(nullptr), nullptr);

        if (!hListBox) {
            MessageBox(hWnd, _T("Не удалось создать список!"), _T("Ошибка"), MB_ICONERROR);
            return -1;
        }

        // Создаем текстовое поле (Edit)
        hEdit = CreateWindow(_T("EDIT"), nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            220, 10, 200, 30, hWnd, (HMENU)101, GetModuleHandle(nullptr), nullptr);

        if (!hEdit) {
            MessageBox(hWnd, _T("Не удалось создать текстовое поле!"), _T("Ошибка"), MB_ICONERROR);
            return -1;
        }

        // Создаем кнопку "Перейти"
        hButtonGo = CreateWindow(_T("BUTTON"), _T("Перейти"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            430, 10, 100, 30, hWnd, (HMENU)102, GetModuleHandle(nullptr), nullptr);

        if (!hButtonGo) {
            MessageBox(hWnd, _T("Не удалось создать кнопку!"), _T("Ошибка"), MB_ICONERROR);
            return -1;
        }

        // Создаем кнопку "Назад"
        hButtonBack = CreateWindow(_T("BUTTON"), _T("Назад"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            540, 10, 100, 30, hWnd, (HMENU)103, GetModuleHandle(nullptr), nullptr);

        if (!hButtonBack) {
            MessageBox(hWnd, _T("Не удалось создать кнопку!"), _T("Ошибка"), MB_ICONERROR);
            return -1;
        }

        // Создаем метку для вывода статуса
        hStatusLabel = CreateWindow(_T("STATIC"), _T(""),
            WS_CHILD | WS_VISIBLE,
            10, 570, 780, 20, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);

        if (!hStatusLabel) {
            MessageBox(hWnd, _T("Не удалось создать метку!"), _T("Ошибка"), MB_ICONERROR);
            return -1;
        }

        // Открываем реестр и получаем список папок в HKEY_LOCAL_MACHINE
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T(""), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD index = 0;
            TCHAR subkey[255];
            while (RegEnumKey(hKey, index, subkey, 255) == ERROR_SUCCESS) {
                subkeys.push_back(subkey);
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)subkey);
                index++;
            }
            RegCloseKey(hKey);
        }
    }
                  break;

    case WM_COMMAND: {
        if (LOWORD(wParam) == 102) { // Кнопка "Перейти"
            TCHAR subkey[255];
            GetWindowText(hEdit, subkey, 255);

            // Перейти в выбранную папку и обновить список
            HKEY hKey;
            if (RegOpenKeyEx(hCurrentKey, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
                subkeys.clear();
                folderStack.push(subkey);
                hCurrentKey = hKey;

                UpdateFolderList(hCurrentKey, subkeys, hListBox, hStatusLabel);

                // Обновляем статус
                SetWindowText(hStatusLabel, _T(""));
            }
            else {
                // Папка не найдена, попробуйте что-то другое
                SetWindowText(hStatusLabel, _T("Папка не найдена"));
            }
        }
        else if (LOWORD(wParam) == 103) { // Кнопка "Назад"
            if (!folderStack.empty()) {
                folderStack.pop();

                // Если стек не пуст, восстанавливаем путь из вершины стека
                if (!folderStack.empty()) {
                    std::wstring path = L"";
                    std::stack<std::wstring> tempStack = folderStack;
                    while (!tempStack.empty()) {
                        path += tempStack.top();
                        tempStack.pop();
                        if (!tempStack.empty()) {
                            path += L"\\";
                        }
                    }
                    RegCloseKey(hCurrentKey);
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hCurrentKey) == ERROR_SUCCESS) {
                        SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
                        subkeys.clear();
                        UpdateFolderList(hCurrentKey, subkeys, hListBox, hStatusLabel);
                    }
                }
                else {
                    // Если стек пуст, восстанавливаем hCurrentKey в HKEY_LOCAL_MACHINE
                    RegCloseKey(hCurrentKey);
                    hCurrentKey = HKEY_LOCAL_MACHINE;
                    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
                    subkeys.clear();
                    UpdateFolderList(hCurrentKey, subkeys, hListBox, hStatusLabel);
                }
            }
        }
    }
                   break;

    case WM_DESTROY:
        // Закрываем текущий ключ реестра перед выходом
        if (hCurrentKey != HKEY_LOCAL_MACHINE) {
            RegCloseKey(hCurrentKey);
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}