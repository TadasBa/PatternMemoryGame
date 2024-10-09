#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "resource.h"

#define GRID_SIZE 3  // Initial grid size
#define MAX_GRID_SIZE 10

HBITMAP hBackgroundBitmap;

// Global variables
int gridSize = GRID_SIZE;
int highlightedBlocks[100];
int playerSelections[100];  // Max player selections
int patternCount = 3;      // Initial number of highlighted blocks
int score = 0;
int currentLevel = 1;      // Track the current level
int playerSelectionCount = 0;  // To count how many blocks the player has selected
HWND hwndScoreDisplay;  // Handle for score display static text
int totalBlocks = gridSize * gridSize; // Define totalBlocks here
const int totalLevels = 3;

HINSTANCE hInst;
HWND hwndStartButton, hwndResetButton, hwndExitButton, hwndLevelDisplay;
BOOL patternShown = FALSE;

// Function Prototypes
void ShowPattern(HWND hwnd);
void DrawGrid(HWND hwnd, HDC hdc);
void CheckPlayerSelections(HWND hwnd);
void NextLevel(HWND hwnd);
void ResetGame(HWND hwnd);
void UpdateLevelDisplay(HWND hwnd);

// Function to create the main window
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

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

    hwnd = CreateWindow(
               "PatternMemoryGame", "Pattern Memory Game",
               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // Static window
               CW_USEDEFAULT, CW_USEDEFAULT, 600, 550,    // Window size
               NULL, NULL, hInstance, NULL);

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

    switch (msg)
    {
    case WM_CREATE:
    {
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

        AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "Settings");
        AppendMenu(hSubMenu, MF_STRING, ID_RESET, "Reset");
        AppendMenu(hSubMenu, MF_STRING, ID_EXIT, "Exit");

        SetMenu(hwnd, hMenu);

        hwndStartButton = CreateWindow(
                              "BUTTON", "Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                              10, 10, 100, 30, hwnd, (HMENU)ID_START, hInst, NULL);

        hwndLevelDisplay = CreateWindow(
                               "STATIC", "Level: ", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_CENTER,
                               380, 10, 100, 30, hwnd, NULL, hInst, NULL);

        // Score display static text
        hwndScoreDisplay = CreateWindow(
                               "STATIC", "Score: 0", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_CENTER,
                               490, 10, 100, 30, hwnd, NULL, hInst, NULL);

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
            int cellX = (x - 100) / 60; // Adjust based on grid position
            int cellY = (y - 100) / 60;

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
        // Draw background
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
        DeleteObject(hBackgroundBitmap); // Clean up
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
    int borderThickness = 5;  // Adjust to increase border size

    for (int row = 0; row < gridSize; row++)
    {
        for (int col = 0; col < gridSize; col++)
        {
            int index = row * gridSize + col;

            // Outer rectangle (border)
            SetRect(&outerRect,
                    100 + col * cellSize,
                    100 + row * cellSize,
                    160 + col * cellSize,
                    160 + row * cellSize);

            // Inner rectangle (inside the border)
            SetRect(&innerRect,
                    100 + col * cellSize + borderThickness,
                    100 + row * cellSize + borderThickness,
                    160 + col * cellSize - borderThickness,
                    160 + row * cellSize - borderThickness);

            // Draw the outer border
            FillRect(hdc, &outerRect, (HBRUSH)GetStockObject(BLACK_BRUSH));  // Black border

            // Draw the inner part based on whether the cell is highlighted, selected, or empty
            if (highlightedBlocks[index] && patternShown)
            {
                FillRect(hdc, &innerRect, CreateSolidBrush(RGB(255, 255, 0)));  // Yellow for highlighted blocks
            }
            else if (playerSelections[index])
            {
                FillRect(hdc, &innerRect, CreateSolidBrush(RGB(0, 255, 0)));  // Green for selected blocks
            }
            else
            {
                FillRect(hdc, &innerRect, (HBRUSH)GetStockObject(WHITE_BRUSH));  // Opaque white background for empty cells
            }
        }
    }
}

// Function to check if player selections are correct
void CheckPlayerSelections(HWND hwnd)
{
    for (int i = 0; i < totalBlocks; i++)
    {
        if (playerSelections[i] == 1 && highlightedBlocks[i] == 1)
        {
            // Correct selection, increase score
            score += 100;
            char scoreText[20];
            sprintf(scoreText, "Score: %d", score);
            SetWindowText(hwndScoreDisplay, scoreText);
        }
        else if (playerSelections[i] == 1 && highlightedBlocks[i] == 0)
        {
            // Incorrect selection, show message and reset game
            MessageBox(hwnd, "Incorrect! Game over!", "Failure", MB_OK);
            ResetGame(hwnd);
            return;
        }
    }

    // If all selections are correct, go to the next level
    if (playerSelectionCount == patternCount)
    {
        if (currentLevel >= totalLevels)    // Check if the player has reached level 5
        {
            MessageBox(hwnd, "You won! Congratulations!", "Game Over", MB_OK | MB_ICONINFORMATION);
            ResetGame(hwnd);  // Restart the game
        }
        else
        {
            MessageBox(hwnd, "Correct! Moving to the next level.", "Success", MB_OK);
            NextLevel(hwnd);
        }
    }
}

// Function to advance to the next level
void NextLevel(HWND hwnd)
{
    if (gridSize < MAX_GRID_SIZE)
    {
        gridSize++;
    }
    patternCount++;

    // Reset selections and increase level
    ZeroMemory(highlightedBlocks, sizeof(highlightedBlocks));
    ZeroMemory(playerSelections, sizeof(playerSelections));
    playerSelectionCount = 0;
    currentLevel++;

    UpdateLevelDisplay(hwnd);
    ShowPattern(hwnd);
}

// Function to reset the game state
void ResetGame(HWND hwnd)
{
    gridSize = GRID_SIZE;
    patternCount = 3;
    score = 0;
    currentLevel = 1;

    SetWindowText(hwndScoreDisplay, "Score: 0");

    ZeroMemory(highlightedBlocks, sizeof(highlightedBlocks));
    ZeroMemory(playerSelections, sizeof(playerSelections));
    playerSelectionCount = 0;

    UpdateLevelDisplay(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);  // Trigger repaint
}

// Function to update the level display
void UpdateLevelDisplay(HWND hwnd)
{
    char levelText[20];
    sprintf(levelText, "Level: %d / %d", currentLevel, totalLevels);
    SetWindowText(hwndLevelDisplay, levelText);
}
