#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "resource.h"

#define GRID_SIZE 3  // Initial grid size
#define MAX_GRID_SIZE 10
#define MAX_TOP_SCORES 5
#define SCORE_FILE "highscores.txt"

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
const int totalLevels = 5;

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
void SaveHighScore(int score);
void LoadHighScores();

// Structure to store scores with timestamps
typedef struct
{
    int score;
    char timestamp[100];  // Stores the date and time
} HighScore;

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
               CW_USEDEFAULT, CW_USEDEFAULT, 600, 590,    // Window size
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

        LoadHighScores();  // Load the top scores at the start of the game
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
    int gridPadding = 50;  // Padding from the edge of the window
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
            SetRect(&outerRect,
                    startX + col * cellSize,
                    startY + row * cellSize,
                    startX + (col + 1) * cellSize,
                    startY + (row + 1) * cellSize);

            // Inner rectangle (inside the border)
            SetRect(&innerRect,
                    startX + col * cellSize + borderThickness,
                    startY + row * cellSize + borderThickness,
                    startX + (col + 1) * cellSize - borderThickness,
                    startY + (row + 1) * cellSize - borderThickness);

            // Check if block is highlighted
            if (highlightedBlocks[index] == 1 && patternShown)
            {
                FillRect(hdc, &outerRect, CreateSolidBrush(RGB(255, 0, 0)));  // Red for pattern
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
            SaveHighScore(score);  // Save the score on failure
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
            SaveHighScore(score);  // Save the score after game completion
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

    // Increase grid size at certain levels (optional)
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

// Function to update level display
void UpdateLevelDisplay(HWND hwnd)
{
    char buffer[50];
    sprintf(buffer, "Level: %d", currentLevel);
    SetWindowText(hwndLevelDisplay, buffer);
}

// Function to save high scores to a file
// Function to save high scores to a file and ensure they are sorted
void SaveHighScore(int newScore)
{
    HighScore scores[MAX_TOP_SCORES + 1];  // One extra slot for the new score
    int scoreCount = 0;

    // Load existing scores from the file
    FILE *file = fopen(SCORE_FILE, "r");
    if (file != NULL)
    {
        while (fscanf(file, "%d %99[^\n]", &scores[scoreCount].score, scores[scoreCount].timestamp) == 2 && scoreCount < MAX_TOP_SCORES)
        {
            scoreCount++;
        }
        fclose(file);
    }

    // Add the new score
    if (scoreCount < MAX_TOP_SCORES || newScore > scores[MAX_TOP_SCORES - 1].score)
    {
        time_t now = time(NULL);
        struct tm *local = localtime(&now);
        char timestamp[100];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local);

        // Insert the new score at the end
        scores[scoreCount].score = newScore;
        strcpy(scores[scoreCount].timestamp, timestamp);
        scoreCount++;
    }

    // Sort scores in descending order
    for (int i = 0; i < scoreCount - 1; i++)
    {
        for (int j = i + 1; j < scoreCount; j++)
        {
            if (scores[i].score < scores[j].score)
            {
                HighScore temp = scores[i];
                scores[i] = scores[j];
                scores[j] = temp;
            }
        }
    }

    // Write back the top 5 scores to the file
    file = fopen(SCORE_FILE, "w");
    if (file != NULL)
    {
        for (int i = 0; i < scoreCount && i < MAX_TOP_SCORES; i++)
        {
            fprintf(file, "%d %s\n", scores[i].score, scores[i].timestamp);
        }
        fclose(file);
    }
    else
    {
        MessageBox(NULL, "Error opening file for high score saving!", "File Error", MB_OK);
    }
}

// Function to load high scores from a file
void LoadHighScores()
{
    FILE *file = fopen(SCORE_FILE, "r");
    if (file != NULL)
    {
        HighScore scores[MAX_TOP_SCORES] = {0};
        int i = 0;

        while (fscanf(file, "%d %99[^\n]", &scores[i].score, scores[i].timestamp) == 2 && i < MAX_TOP_SCORES)
        {
            i++;
        }

        fclose(file);

        // Sort the scores in descending order (optional)

        // Display the high scores (could be printed, shown in a window, etc.)
        char scoreDisplay[500] = "Top 5 Scores:\n";
        for (int j = 0; j < i; j++)
        {
            char entry[100];
            sprintf(entry, "%d - %s\n", scores[j].score, scores[j].timestamp);
            strcat(scoreDisplay, entry);
        }

        MessageBox(NULL, scoreDisplay, "High Scores", MB_OK);
    }
    else
    {
        MessageBox(NULL, "No high score file found or unable to open!", "File Error", MB_OK);
    }
}
