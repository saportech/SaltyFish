#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <FastLED.h>
#include <Game.h>
#include <Player.h>
#include <Audio.h>
#include <FS.h>
#include <LittleFS.h>
#include <SPI.h>
#include <SD.h>

#define NUM_LEDS 6
#define DATA_PIN 27

#define SD_CS          5
#define SPI_MOSI      23    // SD Card
#define SPI_MISO      19
#define SPI_SCK       18

#define I2S_DOUT 33
#define I2S_BCLK 26
#define I2S_LRC 25
#define SD_CS 5

enum SOUND {
    MOVED_SOUND,
    READY_SOUND,
    MISSION_ACCOMPLISHED_SOUND,
};

class UI {
public:
    UI();
    void setup();
    void updateReactions(int gameState, int playerStatus);
    void playSound(SOUND sound);
    void vibrateMotor();
    void setBrightness(uint8_t brightness);
    void resetVibrateFlag();
    void debugSound();

private:
    void updateLEDs(int gameState, int playerStatus);
    void setupAudio();
    CRGB leds[NUM_LEDS];
    Audio audio;
    int vibrationMotorPin1 = 14;
    int vibrationMotorPin2 = 12;
    uint8_t ledBrightness = 10;
    bool motorActivated = false;
    void listLittleFSFiles();   
};

#endif // UI_H
