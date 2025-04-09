#include "Game.h"

Game::Game() {

}

void Game::begin() {
    _state = PRE_GAME; // Initialize state as IDLE by default
    _sensitivity = 9; // Initialize sensitivity as 5 by default
    _gameMode = INDIVIDUAL_MANUAL; // Initialize game mode as INDIVIDUAL_MANUAL by default
    Serial.println("Game initialized with values: ");
    Serial.print("State: ");
    Serial.println(getStateName(_state));
    Serial.print("Sensitivity: ");
    Serial.println(_sensitivity);
    Serial.print("Game Mode: ");
    Serial.println(getGameModeName(_gameMode));

}

void Game::setState(GameState state) {
    _state = state;
}

GameState Game::getState() {
    return _state;
}

void Game::setSensitivity(int sensitivity) {

    if (sensitivity < 1) {
        _sensitivity = 1;
    } else if (sensitivity > 9) {
        _sensitivity = 9;
    } else {
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

String Game::getStateName(GameState state) {
    switch (state) {
        case PRE_GAME: return "PRE_GAME";
        case GAME_BEGIN: return "GAME_BEGIN";
        case RED: return "RED";
        case GREEN: return "GREEN";
        case GAME_OVER: return "GAME_OVER";
        default: return "UNKNOWN";
    }
}

String Game::getGameModeName(GameMode mode) {
    switch (mode) {
        case INDIVIDUAL_MANUAL: return "INDIVIDUAL_MANUAL";
        case INDIVIDUAL_AUTOMATIC: return "INDIVIDUAL_AUTOMATIC";
        case TEAM_MANUAL: return "TEAM_MANUAL";
        case TEAM_AUTOMATIC: return "TEAM_AUTOMATIC";
        default: return "UNKNOWN";
    }
}