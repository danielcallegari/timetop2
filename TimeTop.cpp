// TimeTop.cpp : A presentation timer with progress bar overlay
// (c) Daniel Callegari, 2025
//

#include "framework.h"
#include "TimeTop.h"
#include <windowsx.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")

// Global Variables
HINSTANCE g_hInst;
HWND g_hProgressWnd = nullptr;
DWORD g_dwStartTime = 0;
DWORD g_dwTimeoutMinutes = DEFAULT_TIMEOUT_MINUTES;
COLORREF g_ProgressColor = RGB(0, 120, 215); // Default blue
int g_ProgressThickness = PROGRESS_BAR_THICKNESS;
bool g_bTimerRunning = false;

// Progress window class name
static const wchar_t* PROGRESS_CLASS_NAME = L"TimeTopProgressBar";

// Predefined timeout options
static const DWORD TIMEOUT_OPTIONS[] = { 5, 10, 15, 30, 60, 90, 120 };
static const int TIMEOUT_COUNT = sizeof(TIMEOUT_OPTIONS) / sizeof(TIMEOUT_OPTIONS[0]);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    g_hInst = hInstance;

    // Initialize common controls for tooltips
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Register the progress bar window class
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = ProgressWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TIMETOP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr; // No background
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = PROGRESS_CLASS_NAME;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(nullptr, L"Failed to register window class", L"Error", MB_OK);
        return FALSE;
    }

    // Create the progress window
    CreateProgressWindow();
    
    if (!g_hProgressWnd)
    {
        return FALSE;
    }
    
    // Start the timer immediately with the default timeout
    StartTimer();

    MSG msg;
    // Main message loop
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

void CreateProgressWindow()
{
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    
    // Create a layered window for transparency
    g_hProgressWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        PROGRESS_CLASS_NAME,
        L"TimeTop Progress",
        WS_POPUP,
        0, 0, screenWidth, g_ProgressThickness,
        nullptr, nullptr, g_hInst, nullptr);

    if (!g_hProgressWnd)
    {
        MessageBox(nullptr, L"Failed to create progress window", L"Error", MB_OK);
        return;
    }

    // Set window transparency
    SetLayeredWindowAttributes(g_hProgressWnd, 0, 200, LWA_ALPHA);
    
    // Show the window
    ShowWindow(g_hProgressWnd, SW_SHOW);
    UpdateWindow(g_hProgressWnd);
}

LRESULT CALLBACK ProgressWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            // Set timer for progress updates
            SetTimer(hWnd, TIMER_ID, TIMER_INTERVAL, nullptr);
            break;

        case WM_TIMER:
            if (wParam == TIMER_ID)
            {
                UpdateProgressBar();
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
            
                RECT rect;
                GetClientRect(hWnd, &rect);
            
                // Clear the background first
                HBRUSH hBkBrush = CreateSolidBrush(RGB(50, 50, 50)); // Dark background
                FillRect(hdc, &rect, hBkBrush);
                DeleteObject(hBkBrush);
            
                if (g_bTimerRunning)
                {
                    DWORD remainingMs = GetRemainingTimeMs();
                    DWORD totalMs = g_dwTimeoutMinutes * 60 * 1000;
                
                    if (remainingMs > 0)
                    {
                        // Calculate progress width
                        double progress = (double)remainingMs / (double)totalMs;
                        int progressWidth = (int)(rect.right * progress);
                    
                        // Draw progress bar
                        HBRUSH hBrush = CreateSolidBrush(g_ProgressColor);
                        RECT progressRect = { 0, 0, progressWidth, rect.bottom };
                        FillRect(hdc, &progressRect, hBrush);
                        DeleteObject(hBrush);
                    }
                }
            
                EndPaint(hWnd, &ps);
            }
            break;

        case WM_LBUTTONDOWN:
            {
                // Check if click is in the leftmost 10 pixels
                POINT pt;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            
                if (pt.x <= 10 && g_bTimerRunning)
                {
                    DWORD remainingMs = GetRemainingTimeMs();
                    std::wstring timeStr = L"Remaining time: " + FormatTime(remainingMs);
                    MessageBox(hWnd, timeStr.c_str(), L"TimeTop", MB_OK);
                }
            }
            break;

        case WM_RBUTTONUP:
            {
                POINT pt;
                GetCursorPos(&pt);
                ShowContextMenu(hWnd, pt.x, pt.y);
            }
            break;

        case WM_DESTROY:
            KillTimer(hWnd, TIMER_ID);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


void UpdateProgressBar()
{
    if (!g_bTimerRunning)
        return;

    DWORD remainingMs = GetRemainingTimeMs();
    if (remainingMs == 0)
    {
        // Timer finished
        StopTimer();
        MessageBox(g_hProgressWnd, L"Time's up!", L"TimeTop", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Force redraw of the progress bar
    InvalidateRect(g_hProgressWnd, nullptr, FALSE);
    UpdateWindow(g_hProgressWnd);
}

void ShowContextMenu(HWND hWnd, int x, int y)
{
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, IDM_RESTART, L"&Restart Timer");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_SETTINGS, L"&Settings...");
    AppendMenu(hMenu, MF_STRING, IDM_ABOUT_CTX, L"&About...");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT_CTX, L"E&xit");

    int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, x, y, 0, hWnd, nullptr);
    
    switch (cmd)
        {
        case IDM_RESTART:
            StartTimer();
            break;
        case IDM_SETTINGS:
            ShowSettingsDialog();
            break;
        case IDM_ABOUT_CTX:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlgProc);
            break;
        case IDM_EXIT_CTX:
            DestroyWindow(hWnd);
            break;
    }
    
    DestroyMenu(hMenu);
}

void ShowSettingsDialog()
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SETTINGS), g_hProgressWnd, SettingsDlgProc);
}


/*****************************************
    TIMER
*****************************************/

void StartTimer()
{
    g_dwStartTime = GetTickCount();
    g_bTimerRunning = true;
    InvalidateRect(g_hProgressWnd, nullptr, TRUE);
}

void StopTimer()
{
    g_bTimerRunning = false;
    InvalidateRect(g_hProgressWnd, nullptr, TRUE);
}

DWORD GetRemainingTimeMs()
{
    if (!g_bTimerRunning)
        return 0;
        
    DWORD elapsed = GetTickCount() - g_dwStartTime;
    DWORD totalMs = g_dwTimeoutMinutes * 60 * 1000;
    
    if (elapsed >= totalMs)
        return 0;
        
    return totalMs - elapsed;
}

std::wstring FormatTime(DWORD timeMs)
{
    DWORD totalSeconds = timeMs / 1000;
    DWORD minutes = totalSeconds / 60;
    DWORD seconds = totalSeconds % 60;
    
    std::wstringstream ss;

    // Less than 5 minutes? display MM:SS
    if (minutes < 5) {
        ss << std::setfill(L'0') << std::setw(2) << minutes << L":"
           << std::setfill(L'0') << std::setw(2) << seconds;
    }
    else {
        ss << minutes << L" minutes.";
    }

    return ss.str();
}

/*****************************************
    DIALOGS
*****************************************/

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Populate timeout combo box
            HWND hCombo = GetDlgItem(hDlg, IDC_TIMEOUT_COMBO);
            for (int i = 0; i < TIMEOUT_COUNT; i++)
            {
                std::wstringstream ss;
                ss << TIMEOUT_OPTIONS[i] << L" minutes";
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)ss.str().c_str());
                if (TIMEOUT_OPTIONS[i] == g_dwTimeoutMinutes)
                {
                    SendMessage(hCombo, CB_SETCURSEL, i, 0);
                }
            }
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Custom...");
            
            // Set color radio buttons
            CheckDlgButton(hDlg, IDC_COLOR_BLUE, (g_ProgressColor == RGB(0, 120, 215)) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_COLOR_RED, (g_ProgressColor == RGB(255, 0, 0)) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_COLOR_GREEN, (g_ProgressColor == RGB(0, 255, 0)) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_COLOR_ORANGE, (g_ProgressColor == RGB(255, 165, 0)) ? BST_CHECKED : BST_UNCHECKED);
            
            // Default to blue if no match
            if (!IsDlgButtonChecked(hDlg, IDC_COLOR_BLUE) && !IsDlgButtonChecked(hDlg, IDC_COLOR_RED) &&
                !IsDlgButtonChecked(hDlg, IDC_COLOR_GREEN) && !IsDlgButtonChecked(hDlg, IDC_COLOR_ORANGE))
            {
                CheckDlgButton(hDlg, IDC_COLOR_BLUE, BST_CHECKED);
            }
            
            // Set thickness
            SetDlgItemInt(hDlg, IDC_THICKNESS_EDIT, g_ProgressThickness, FALSE);
            
            // Set up spin control with range limits
            HWND hSpin = GetDlgItem(hDlg, IDC_THICKNESS_SPIN);
            SendMessage(hSpin, UDM_SETRANGE32, 2, 32);
            SendMessage(hSpin, UDM_SETPOS32, 0, g_ProgressThickness);
            
            return (INT_PTR)TRUE;
        }

    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                {
                    // Get timeout
                    HWND hCombo = GetDlgItem(hDlg, IDC_TIMEOUT_COMBO);
                    int sel = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                    if (sel >= 0 && sel < TIMEOUT_COUNT)
                    {
                        g_dwTimeoutMinutes = TIMEOUT_OPTIONS[sel];
                    }
                    else if (sel == TIMEOUT_COUNT) // Custom
                    {
                        int customTimeout = GetDlgItemInt(hDlg, IDC_CUSTOM_TIMEOUT, nullptr, FALSE);
                        if (customTimeout > 0 && customTimeout <= 999)
                        {
                            g_dwTimeoutMinutes = customTimeout;
                        }
                    }
                    
                    // Get color
                    if (IsDlgButtonChecked(hDlg, IDC_COLOR_RED))
                        g_ProgressColor = RGB(255, 0, 0);
                    else if (IsDlgButtonChecked(hDlg, IDC_COLOR_GREEN))
                        g_ProgressColor = RGB(0, 255, 0);
                    else if (IsDlgButtonChecked(hDlg, IDC_COLOR_ORANGE))
                        g_ProgressColor = RGB(255, 165, 0);
                    else
                        g_ProgressColor = RGB(0, 120, 215); // Default blue
                    
                    // Get thickness
                    int thickness = GetDlgItemInt(hDlg, IDC_THICKNESS_EDIT, nullptr, FALSE);
                    if (thickness >= 2 && thickness <= 32)
                    {
                        g_ProgressThickness = thickness;
                        
                        // Resize window for new thickness
                        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                        SetWindowPos(g_hProgressWnd, HWND_TOPMOST, 0, 0, screenWidth, g_ProgressThickness,
                                   SWP_NOACTIVATE | SWP_SHOWWINDOW);
                        
                    }
                    else
                    {
                        // Show error message and don't close dialog
                        MessageBox(hDlg, L"Thickness must be between 2 and 32 pixels.", L"Invalid Input", MB_OK | MB_ICONWARNING);
                        return (INT_PTR)TRUE; // Stay in dialog
                    }
                    
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
                
            case IDCANCEL:
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
