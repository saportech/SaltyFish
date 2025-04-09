#ifndef Game_h
#define Game_h

#include <Arduino.h>

enum GameState {
    PRE_GAME,
    GAME_BEGIN,
    RED,
    GREEN,
    GAME_OVER
};

enum GameMode {
    INDIVIDUAL_AUTOMATIC,
    INDIVIDUAL_MANUAL,
    TEAM_AUTOMATIC,
    TEAM_MANUAL
};

class Game {
public:
    Game();
    void setState(GameState state);
    GameState getState();
    void setSensitivity(int sensitivity);
    int getSensitivity();
    void setGameMode(GameMode mode);
    GameMode getGameMode();
private:
    GameState _state;
    int _sensitivity;
    GameMode _gameMode;
};

#endif
