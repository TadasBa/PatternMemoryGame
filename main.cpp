#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "resource.h"

#define GRID_SIZE 3  // Initial grid size
#define MAX_GRID_SIZE 10
#define MAX_TOP_SCORES 5
#define SCORE_FILE "highscores.txt"

// Global variables
int gridSize = GRID_SIZE;
int highlightedBlocks[100];
int playerSelections[100];
int patternCount = 3;      // Initial number of highlighted blocks
int score = 0;
int currentLevel = 1;
int playerSelectionCount = 0;  // To count how many blocks the player has selected
HWND hwndScoreDisplay;  // Handle for score display static text
int totalBlocks = gridSize * gridSize; // Define totalBlocks here
const int totalLevels = 5;
HBITMAP hBackgroundBitmap;
HINSTANCE hInst;
HWND hwndStartButton, hwndLevelDisplay;
BOOL patternShown = FALSE;

// Function Prototypes
void ShowPattern(HWND hwnd);
void DrawGrid(HWND hwnd, HDC hdc);
void CheckPlayerSelections(HWND hwnd);
void NextLevel(HWND hwnd);
void ResetGame(HWND hwnd);
void UpdateScoreDisplay(HWND hwnd);
void UpdateLevelDisplay(HWND hwnd);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // Function to create the main window


// Declare function pointers for the DLL functions
typedef void (*SaveHighScoreFunc)(int newScore);
typedef void (*LoadHighScoresFunc)();
typedef void (*SomeFunctionFunc)(LPCSTR sometext);

// Global function pointers
SaveHighScoreFunc pSaveHighScore = nullptr;
LoadHighScoresFunc pLoadHighScores = nullptr;
SomeFunctionFunc pSomeFunction = nullptr;


typedef void (__cdecl *MYPROC)(LPCSTR);


// Structure to store scores with timestamps
typedef struct
{
    int score;
    char timestamp[100];  // Stores the date and time of specific score
} HighScore;


// Main program entry
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc = {0};
    HWND hwnd;
    MSG msg;

    hInst = hInstance;  // Initialize hInst here

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PatternMemoryGame";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);

    RegisterClass(&wc);

    hwnd = CreateWindow("PatternMemoryGame", "Pattern Memory Game", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 600, 590, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

// Window procedure function
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    HDC hMemDC;
    PAINTSTRUCT ps;
    HMODULE hDll;

    switch (msg)
    {
    case WM_CREATE:
    {
        // Load the DLL
        hDll = LoadLibrary("PatternGameDLL.dll");
        if (hDll)
        {
            // Get the addresses of the functions from the DLL
            pSaveHighScore = (SaveHighScoreFunc)GetProcAddress(hDll, "SaveHighScore");
            pLoadHighScores = (LoadHighScoresFunc)GetProcAddress(hDll, "LoadHighScores");
            pSomeFunction = (SomeFunctionFunc)GetProcAddress(hDll, "SomeFunction");

            // Check if the function pointers were successfully loaded
            if (!pSaveHighScore || !pLoadHighScores || !pSomeFunction)
            {
                MessageBox(NULL, "Could not load DLL functions!", "Error", MB_OK | MB_ICONERROR);
            }
        }
        else
        {
            MessageBox(NULL, "Could not load DLL!", "Error", MB_OK | MB_ICONERROR);
        }

        hBackgroundBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BACKGROUND)); // Load background
        if (hBackgroundBitmap == NULL)
        {
            char errorMsg[100];
            sprintf(errorMsg, "Failed to load background bitmap. Error: %lu", GetLastError());
            MessageBox(hwnd, errorMsg, "Error", MB_OK | MB_ICONERROR);
        }


        // Create the Settings Menu
        HMENU hMenu = CreateMenu();
        HMENU hSubMenu = CreatePopupMenu();

        AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "Menu");
        AppendMenu(hSubMenu, MF_STRING, ID_RESET, "Reset");
        AppendMenu(hSubMenu, MF_STRING, ID_EXIT, "Exit");
        AppendMenu(hSubMenu, MF_STRING, ID_SCORES, "High Scores");

        SetMenu(hwnd, hMenu);

        hwndStartButton = CreateWindow("BUTTON", "Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, 100, 30, hwnd, (HMENU)ID_START, hInst, NULL);
        hwndLevelDisplay = CreateWindow("STATIC", "Level: ", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_CENTER, 380, 10, 100, 30, hwnd, NULL, hInst, NULL);
        hwndScoreDisplay = CreateWindow("STATIC", "Score: 0", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_CENTER, 490, 10, 100, 30, hwnd, NULL, hInst, NULL);
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_START:
            ResetGame(hwnd);  // Start fresh
            ShowPattern(hwnd);
            break;
        case ID_RESET:
            ResetGame(hwnd);  // Reset to initial state
            break;
        case ID_EXIT:
            PostQuitMessage(0);  // Exit the game
            break;
        case ID_SCORES:
            pLoadHighScores();  // Load the top scores at the start of the game
            break;
        }
        break;
    }

    case WM_TIMER:
    {
        if (wParam == ID_TIMER_HIGHLIGHT)
        {
            KillTimer(hwnd, ID_TIMER_HIGHLIGHT);
            patternShown = FALSE;  // Player can now start selecting
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }

    case WM_LBUTTONDOWN:
    {
        if (!patternShown)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // Use the same startX and startY as in DrawGrid
            int cellSize = 60;
            int startX = (600 - (gridSize * cellSize)) / 2;
            int startY = 100;  // Keep it the same as in DrawGrid

            // Calculate which cell was clicked
            int cellX = (x - startX) / cellSize;
            int cellY = (y - startY) / cellSize;

            // Ensure the selected cell is within bounds
            if (cellX >= 0 && cellX < gridSize && cellY >= 0 && cellY < gridSize)
            {
                int index = cellY * gridSize + cellX;

                // Check if the player hasn't already selected this block
                if (playerSelections[index] == 0)
                {
                    playerSelections[index] = 1;
                    playerSelectionCount++; // Increase the selection count
                    InvalidateRect(hwnd, NULL, TRUE); // Repaint the grid

                    // Check the pattern only after all required blocks are selected
                    if (playerSelectionCount == patternCount)
                    {
                        CheckPlayerSelections(hwnd);
                    }
                }
            }
        }
        break;
    }


    case WM_PAINT:
    {
        hdc = BeginPaint(hwnd, &ps);
        hMemDC = CreateCompatibleDC(hdc);
        SelectObject(hMemDC, hBackgroundBitmap);
        BITMAP bitmap;
        GetObject(hBackgroundBitmap, sizeof(BITMAP), &bitmap);
        BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hMemDC, 0, 0, SRCCOPY);
        DeleteDC(hMemDC);
        DrawGrid(hwnd, hdc); // Draw the grid on top of the background
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
    {
        // Clean up
        DeleteObject(hBackgroundBitmap);
        FreeLibrary(hDll);
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

// Function to show the pattern to the player
void ShowPattern(HWND hwnd)
{
    patternShown = TRUE;
    srand(time(NULL));
    int totalCells = gridSize * gridSize;

    // Clear previous selections and highlight new random blocks
    ZeroMemory(highlightedBlocks, sizeof(highlightedBlocks));

    for (int i = 0; i < patternCount; i++)
    {
        int randomIndex;
        do
        {
            randomIndex = rand() % totalCells;
        }
        while (highlightedBlocks[randomIndex] == 1);

        highlightedBlocks[randomIndex] = 1;
    }

    SetTimer(hwnd, ID_TIMER_HIGHLIGHT, 3000, NULL);  // Show pattern for 3 seconds
    InvalidateRect(hwnd, NULL, TRUE);  // Repaint grid to show highlighted blocks
}

// Function to draw the grid and highlighted blocks
void DrawGrid(HWND hwnd, HDC hdc)
{
    RECT outerRect, innerRect;
    int cellSize = 60;
    int borderThickness = 5;  // Border size

    // Calculate the starting X and Y coordinates for centering
    int startX = (600 - (gridSize * cellSize)) / 2;
    int startY = 100;  // Keep it a bit lower to allow for header and score display

    for (int row = 0; row < gridSize; row++)
    {
        for (int col = 0; col < gridSize; col++)
        {
            int index = row * gridSize + col;

            // Outer rectangle (border)
            SetRect(&outerRect, startX + col * cellSize, startY + row * cellSize, startX + (col + 1) * cellSize, startY + (row + 1) * cellSize);

            // Inner rectangle (inside the border)
            SetRect(&innerRect, startX + col * cellSize + borderThickness, startY + row * cellSize + borderThickness, startX + (col + 1) * cellSize - borderThickness, startY + (row + 1) * cellSize - borderThickness);

            // Check if block is highlighted
            if (highlightedBlocks[index] == 1 && patternShown)
            {
                FillRect(hdc, &outerRect, CreateSolidBrush(RGB(255,222,33)));  // Yellow for pattern
            }
            else if (playerSelections[index] == 1)
            {
                FillRect(hdc, &outerRect, CreateSolidBrush(RGB(0, 255, 0)));  // Green for selected
            }
            else
            {
                FillRect(hdc, &outerRect, CreateSolidBrush(RGB(255, 255, 255)));  // Default white
            }

            // Draw grid cell border
            FrameRect(hdc, &outerRect, CreateSolidBrush(RGB(0, 0, 0)));
        }
    }
}

// Function to check if player's selections match the pattern
void CheckPlayerSelections(HWND hwnd)
{
    int correctSelections = 0;
    char buffer[100];

    for (int i = 0; i < gridSize * gridSize; i++)
    {
        if (playerSelections[i] == 1 && highlightedBlocks[i] == 1)
        {
            correctSelections++;
        }
        else if (playerSelections[i] == 1 && highlightedBlocks[i] != 1)
        {
            sprintf(buffer, "Wrong block selected! Game Over. Your score: %d", score);
            MessageBox(hwnd, buffer, "Game Over", MB_OK);
            pSaveHighScore(score);  // Save the score on failure
            ResetGame(hwnd);
            return;
        }
    }

    // If all selections are correct
    if (correctSelections == patternCount)
    {
        score += 100 * patternCount;  // Add score for correct selections
        UpdateScoreDisplay(hwnd);  // Update the score display

        if (currentLevel == totalLevels)
        {
            sprintf(buffer, "Congratulations! You have completed all levels. Your score: %d", score);
            MessageBox(hwnd, buffer, "Game Complete", MB_OK);
            pSaveHighScore(score);  // Save the score after game completion
            ResetGame(hwnd);
        }
        else
        {
            MessageBox(hwnd, "Correct! Advancing to the next level.", "Success", MB_OK);
            NextLevel(hwnd);  // Move to next level
        }
    }
}

// Function to move to the next level
void NextLevel(HWND hwnd)
{
    currentLevel++;
    patternCount++;

    if (gridSize < MAX_GRID_SIZE)
        gridSize++;

    // Update the level and score displays
    UpdateLevelDisplay(hwnd);
    char scoreBuffer[50];
    sprintf(scoreBuffer, "Score: %d", score);
    SetWindowText(hwndScoreDisplay, scoreBuffer);

    // Reset player selections
    ZeroMemory(playerSelections, sizeof(playerSelections));
    playerSelectionCount = 0;

    ShowPattern(hwnd);  // Show new pattern
}

// Function to reset the game
void ResetGame(HWND hwnd)
{
    currentLevel = 1;
    patternCount = 3;
    score = 0;
    gridSize = GRID_SIZE;

    // Reset the level and score displays
    UpdateLevelDisplay(hwnd);
    SetWindowText(hwndScoreDisplay, "Score: 0");

    ZeroMemory(playerSelections, sizeof(playerSelections));
    ZeroMemory(highlightedBlocks, sizeof(highlightedBlocks));
    playerSelectionCount = 0;
    InvalidateRect(hwnd, NULL, TRUE);  // Repaint the grid
}

void UpdateScoreDisplay(HWND hwnd)
{
    char scoreBuffer[50];
    sprintf(scoreBuffer, "Score: %d", score);
    SetWindowText(hwndScoreDisplay, scoreBuffer);
}

void UpdateLevelDisplay(HWND hwnd)
{
    char buffer[50];
    sprintf(buffer, "Level: %d", currentLevel);
    SetWindowText(hwndLevelDisplay, buffer);
}
