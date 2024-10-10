# Pattern Memory Game

## Overview
This is a simple memory pattern game developed using the Windows API. The game involves a grid of blocks where some blocks are highlighted for a few seconds. The player must memorize and select the highlighted blocks in the correct order. The grid size and the number of highlighted blocks increase with each level. The game tracks the player's score and saves the top 5 scores along with the date and time they were achieved.

## Features
- **Grid of Blocks**: A grid of blocks where some blocks are highlighted.
- **Level Progression**: As the player successfully selects the correct blocks, the game advances to the next level.
- **Score Tracking**: Tracks the player's score based on the number of correctly selected blocks.
- **Top 5 Scores**: Saves the top 5 scores to a file and displays them with the date and time they were achieved.
- **Menu**: Includes options to reset, exit, and view the top scores.
- **Dynamic Link Library (DLL)**: Key functions such as saving and loading high scores are implemented in a separate DLL to keep the code modular.

## Installation
1. Clone the repository to your local machine:
    ```bash
    git clone https://github.com/YourUsername/PatternMemoryGame.git
    ```

2. Open the project in your preferred C++ IDE (e.g., Code::Blocks).

3. Build and run the project. Make sure the **PatternGameDLL.dll** file is located in the same directory as the executable.

## DLL Information
- The **PatternGameDLL.dll** file is crucial for handling the high score functionality, including saving and loading scores.
- Ensure that the DLL is properly linked during compilation and placed in the executable's directory to avoid runtime errors.

## Usage
- Click the **Start** button to begin the game.
- Memorize the highlighted blocks and click on the blocks in the correct order.
- After completing a level, the game will advance to the next one.
- You can view the top 5 scores and the date/time of each achievement from the **Menu**.

