#include <Windows.h>
#include <psapi.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <fstream> 
#include <thread>
#include <future>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MemoryManagerWindowClass";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"MemoryManagerWindowClass", L"Memory Manager",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void UpdateMemoryInfo(HWND hwnd) {
    DWORD processes[1024];
    DWORD cbNeeded;
    if (EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
        int numProcesses = cbNeeded / sizeof(DWORD);
        std::vector<std::pair<std::wstring, DWORD>> processInfo;

        for (int i = 0; i < numProcesses; i++) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
            if (hProcess != NULL) {
                TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

                HMODULE hModule;
                DWORD cbNeeded;
                if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded)) {
                    GetModuleBaseName(hProcess, hModule, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
                }

                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    processInfo.push_back(std::make_pair(szProcessName, pmc.WorkingSetSize));
                }
                CloseHandle(hProcess);
            }
        }

        SendMessage(hwnd, LB_RESETCONTENT, 0, 0);
        for (const auto& info : processInfo) {
            std::wstring processData = info.first;
            processData += L" - ";
            processData += std::to_wstring(info.second / (1024 * 1024));
            processData += L" MB";
            SendMessage(hwnd, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(processData.c_str()));
        }
    }

}

std::future<void> SaveProcessListAsync() {
    return std::async(std::launch::async, []() {
        std::wofstream fileStream(L"ProcessList.txt");
        if (fileStream.is_open()) {
            DWORD processes[1024];
            DWORD cbNeeded;
            if (EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
                int numProcesses = cbNeeded / sizeof(DWORD);

                for (int i = 0; i < numProcesses; i++) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
                    if (hProcess != NULL) {
                        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

                        HMODULE hModule;
                        DWORD cbNeeded;
                        if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded)) {
                            GetModuleBaseName(hProcess, hModule, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
                        }

                        PROCESS_MEMORY_COUNTERS pmc;
                        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                            fileStream << L"Process Name: " << szProcessName << L" - Process ID: " << processes[i]
                                << L" - Memory Usage: " << pmc.WorkingSetSize / (1024 * 1024) << L" MB" << std::endl;
                        }
                        CloseHandle(hProcess);
                    }
                }
            }
            fileStream.close();
        }
        });
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hListBox;
    static DWORD processes[1024];
    static int numProcesses = 0;

    switch (uMsg) {

    case WM_CREATE:
        hListBox = CreateWindowEx(0, L"LISTBOX", L"Process Memory Usage",
            WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_STANDARD, 10, 40, 400, 400, hwnd, (HMENU)1, NULL, NULL);
        UpdateMemoryInfo(hListBox);

        CreateWindow(L"BUTTON", L"Обновить", WS_CHILD | WS_VISIBLE, 430, 10, 100, 25, hwnd, (HMENU)2, NULL, NULL);
        CreateWindow(L"BUTTON", L"Завершить процесс", WS_CHILD | WS_VISIBLE, 220, 10, 200, 25, hwnd, (HMENU)3, NULL, NULL);
        CreateWindow(L"BUTTON", L"Записать в файл", WS_CHILD | WS_VISIBLE, 10, 10, 200, 25, hwnd, (HMENU)4, NULL, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 2:
            UpdateMemoryInfo(hListBox);
            break;
        case 4:
            SaveProcessListAsync();
            break;
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
