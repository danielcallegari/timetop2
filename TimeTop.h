#pragma once

#include "resource.h"
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>

// Timer and UI constants
#define TIMER_ID 1
#define TIMER_INTERVAL 1000  // Update every x milisec
#define DEFAULT_TIMEOUT_MINUTES 90
#define PROGRESS_BAR_THICKNESS 4

// Global variables
extern HINSTANCE g_hInst;
extern HWND g_hProgressWnd;
extern DWORD g_dwStartTime;
extern DWORD g_dwTimeoutMinutes;
extern COLORREF g_ProgressColor;
extern int g_ProgressThickness;
extern bool g_bTimerRunning;

// Function declarations
LRESULT CALLBACK ProgressWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void CreateProgressWindow();
void UpdateProgressBar();
void ShowContextMenu(HWND hWnd, int x, int y);
void ShowSettingsDialog();
void StartTimer();
void StopTimer();
DWORD GetRemainingTimeMs();
std::wstring FormatTime(DWORD timeMs);
