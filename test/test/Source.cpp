#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <psapi.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int currentScrollPos = 0;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("TaskManager"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("Диспетчер задач"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 700, NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

void AnalyzeProcessEfficiency(DWORD processId, HWND resultText) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess != NULL) {
        FILETIME createTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
            ULONGLONG userTimeMS = userTime.dwHighDateTime * 1000000000LL + userTime.dwLowDateTime;
            ULONGLONG kernelTimeMS = kernelTime.dwHighDateTime * 1000000000LL + kernelTime.dwLowDateTime;

            TCHAR result[256];
            _stprintf_s(result, _T("User Time (ms): %lld \n"), userTimeMS / 10000);
            SendMessage(resultText, EM_REPLACESEL, 0, (LPARAM)result);

            _stprintf_s(result, _T("Kernel Time (ms): %lld \n"), kernelTimeMS / 10000);
            SendMessage(resultText, EM_REPLACESEL, 0, (LPARAM)result);
        }

        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            TCHAR result[256];
            _stprintf_s(result, _T("Private Working Set (MB): %.2f \n"), static_cast<double>(pmc.PrivateUsage) / (1024 * 1024));
            SendMessage(resultText, EM_REPLACESEL, 0, (LPARAM)result);

            _stprintf_s(result, _T("Pagefile Usage (MB): %.2f\n"), static_cast<double>(pmc.PagefileUsage) / (1024 * 1024));
            SendMessage(resultText, EM_REPLACESEL, 0, (LPARAM)result);
        }

        /*  User Time(время пользователя в миллисекундах) : Это время, в течение которого процесс был в режиме пользователя(т.е., выполнял свой код вне ядра операционной системы)

            Kernel Time(время ядра в миллисекундах) : Это время, в течение которого процесс выполнял код внутри ядра операционной системы.
            Это включает в себя время, затраченное на обращение к системным ресурсам, таким как дисководы и файловые операции.

            Private Working Set(МБ) : Это объем оперативной памяти, используемый процессом в МБ.
            Этот объем включает в себя физическую память, зарезервированную и используемую только этим процессом.

            Pagefile Usage(МБ) : Это объем памяти, используемый процессом в файлах подкачки операционной системы(pagefile) в МБ.
            Эта память используется, когда физическая оперативная память исчерпана.*/


        CloseHandle(hProcess);
    }
}

void LaunchProcess(const TCHAR* exePath) {
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (CreateProcess(exePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

DWORD FindProcessId(const TCHAR* processName) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (_tcsicmp(pe32.szExeFile, processName) == 0) {
                CloseHandle(hProcessSnap);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
    return 0;

}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HWND processList = CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_STANDARD, 10, 10, 250, 300, hwnd, (HMENU)1, NULL, NULL);

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (Process32First(hProcessSnap, &pe32)) {
            do {
                SendMessage(processList, LB_ADDSTRING, 0, (LPARAM)pe32.szExeFile);
            } while (Process32Next(hProcessSnap, &pe32));
        }
        CloseHandle(hProcessSnap);

        HWND killButton = CreateWindow(_T("BUTTON"), _T("Завершить процесс"), WS_CHILD | WS_VISIBLE, 270, 10, 150, 30, hwnd, (HMENU)2, NULL, NULL);
        HWND refreshButton = CreateWindow(_T("BUTTON"), _T("Обновить список"), WS_CHILD | WS_VISIBLE, 270, 50, 150, 30, hwnd, (HMENU)3, NULL, NULL);
        HWND suspendButton = CreateWindow(_T("BUTTON"), _T("Приостановить процесс"), WS_CHILD | WS_VISIBLE, 270, 90, 150, 30, hwnd, (HMENU)4, NULL, NULL);
        HWND resumeButton = CreateWindow(_T("BUTTON"), _T("Возобновить процесс"), WS_CHILD | WS_VISIBLE, 270, 130, 150, 30, hwnd, (HMENU)5, NULL, NULL);
        HWND createProcessButton = CreateWindow(_T("BUTTON"), _T("Запустить процесс"), WS_CHILD | WS_VISIBLE, 270, 170, 150, 30, hwnd, (HMENU)6, NULL, NULL);
        HWND analyzeButton = CreateWindow(_T("BUTTON"), _T("Исследовать процесс"), WS_CHILD | WS_VISIBLE, 270, 210, 150, 30, hwnd, (HMENU)7, NULL, NULL);
        HWND resultText = CreateWindow(L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY, 10, 320, 800, 150, hwnd, (HMENU)8, NULL, NULL);

        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == 2) {
            HWND processList = GetDlgItem(hwnd, 1);
            int selectedIndex = SendMessage(processList, LB_GETCURSEL, 0, 0);
            if (selectedIndex != LB_ERR) {
                TCHAR processName[260];
                SendMessage(processList, LB_GETTEXT, selectedIndex, (LPARAM)processName);

                int currentScrollPos = SendMessage(processList, LB_GETTOPINDEX, 0, 0);

                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, FindProcessId(processName));
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 0);
                    
                    SendMessage(processList, LB_RESETCONTENT, 0, 0);

                    PROCESSENTRY32 pe32;
                    pe32.dwSize = sizeof(PROCESSENTRY32);
                    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                    if (Process32First(hProcessSnap, &pe32)) {
                        do {
                            SendMessage(processList, LB_ADDSTRING, 0, (LPARAM)pe32.szExeFile);
                        } while (Process32Next(hProcessSnap, &pe32));
                    }
                    CloseHandle(hProcessSnap);
                    CloseHandle(hProcess);
                    SendMessage(processList, LB_SETTOPINDEX, currentScrollPos, 0);
                }
            }
        }
        else if (wmId == 3) {
            HWND processList = GetDlgItem(hwnd, 1);
            currentScrollPos = SendMessage(processList, LB_GETTOPINDEX, 0, 0);

            SendMessage(processList, LB_RESETCONTENT, 0, 0); 

            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (Process32First(hProcessSnap, &pe32)) {
                do {
                    SendMessage(processList, LB_ADDSTRING, 0, (LPARAM)pe32.szExeFile);
                } while (Process32Next(hProcessSnap, &pe32));
            }
            CloseHandle(hProcessSnap);

            SendMessage(processList, LB_SETTOPINDEX, currentScrollPos, 0);
        }
        else if (wmId == 4) {
            HWND processList = GetDlgItem(hwnd, 1);
            int selectedIndex = SendMessage(processList, LB_GETCURSEL, 0, 0);
            if (selectedIndex != LB_ERR) {
                TCHAR processName[260];
                SendMessage(processList, LB_GETTEXT, selectedIndex, (LPARAM)processName);

                int currentScrollPos = SendMessage(processList, LB_GETTOPINDEX, 0, 0);

                HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, FindProcessId(processName));
                if (hProcess != NULL) {
                    SuspendThread(hProcess);
                    CloseHandle(hProcess);
                }
            }
        }
        else if (wmId == 5) {
            HWND processList = GetDlgItem(hwnd, 1);
            int selectedIndex = SendMessage(processList, LB_GETCURSEL, 0, 0);
            if (selectedIndex != LB_ERR) {
                TCHAR processName[260];
                SendMessage(processList, LB_GETTEXT, selectedIndex, (LPARAM)processName);

                int currentScrollPos = SendMessage(processList, LB_GETTOPINDEX, 0, 0);

                HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, FindProcessId(processName));
                if (hProcess != NULL) {
                    ResumeThread(hProcess);
                    CloseHandle(hProcess);
                }
            }
        }
        else if (wmId == 6) {
            OPENFILENAME ofn;
            TCHAR szFileName[MAX_PATH] = { 0 };

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = _T("Исполняемые файлы (*.exe)\0*.exe\0");
            ofn.lpstrFile = szFileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                LaunchProcess(szFileName);

                HWND processList = GetDlgItem(hwnd, 1);
                SendMessage(processList, LB_RESETCONTENT, 0, 0);

                PROCESSENTRY32 pe32;
                pe32.dwSize = sizeof(PROCESSENTRY32);
                HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (Process32First(hProcessSnap, &pe32)) {
                    do {
                        SendMessage(processList, LB_ADDSTRING, 0, (LPARAM)pe32.szExeFile);
                    } while (Process32Next(hProcessSnap, &pe32));
                }
                CloseHandle(hProcessSnap);
            }
        }
        else if (wmId == 7) {
            HWND resultText = GetDlgItem(hwnd, 8);
            SetWindowText(resultText, L"");

            HWND processList = GetDlgItem(hwnd, 1);
            int selectedIndex = SendMessage(processList, LB_GETCURSEL, 0, 0);
            if (selectedIndex != LB_ERR) {
                TCHAR processName[260];
                SendMessage(processList, LB_GETTEXT, selectedIndex, (LPARAM)processName);

                DWORD processId = FindProcessId(processName);

                if (processId != 0) {

                    HWND resultText = GetDlgItem(hwnd, 8);
                    AnalyzeProcessEfficiency(processId, resultText);
                }
            }

        }

        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

