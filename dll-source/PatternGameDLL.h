#ifndef _PATTERN_GAME_DLL_
#define _PATTERN_GAME_DLL_

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void DLL_EXPORT SomeFunction(const LPCSTR sometext);
void DLL_EXPORT SaveHighScore(int newScore);
void DLL_EXPORT LoadHighScores();

#ifdef __cplusplus
}
#endif

#endif // _PATTERN_GAME_DLL_
