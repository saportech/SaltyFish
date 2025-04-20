#ifndef Game_h
#define Game_h

#include <Arduino.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>

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
    void begin();
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
    friend class MyAdvertisedDeviceCallbacks; // Allow the callback class to access private members
    String getStateName(GameState state);
    String getGameModeName(GameMode mode);
    
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
    MyAdvertisedDeviceCallbacks(Game* game) : game(game) {} // Constructor to take Game instance
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
private:
    Game* game; // Reference to the Game instance
};

#endif
