#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "PatternGameDLL.h"

#define MAX_TOP_SCORES 5
#define SCORE_FILE "highscores.txt"

// Structure to store scores with timestamps
typedef struct
{
    int score;
    char timestamp[100];  // Stores the date and time
} HighScore;

// Function to save high scores to a file and ensure they are sorted
void DLL_EXPORT SaveHighScore(int newScore)
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
void DLL_EXPORT LoadHighScores()
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

        // Display the high scores
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


extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // successful
}
