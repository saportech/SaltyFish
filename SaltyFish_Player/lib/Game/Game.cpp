#include "Game.h"

Game::Game() {
    _state = PRE_GAME; // Initialize state as PRE_GAME by default
    _sensitivity = 7; // Initialize sensitivity as 7 by default
    _gameMode = INDIVIDUAL_AUTOMATIC; // Initialize game mode as INDIVIDUAL_AUTOMATIC by default
}

void Game::setState(GameState state) {
    _state = state;
}

GameState Game::getState() {
    return _state;
}

void Game::setSensitivity(int sensitivity) {

    if (sensitivity >= 1 && sensitivity <= 9) {
        _sensitivity = sensitivity;
    }
}

int Game::getSensitivity() {
    return _sensitivity;
}

void Game::setGameMode(GameMode mode) {
    _gameMode = mode;
}

GameMode Game::getGameMode() {
    return _gameMode;
}
