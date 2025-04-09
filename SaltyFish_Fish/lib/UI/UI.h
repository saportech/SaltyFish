#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Game.h"
#include "Player.h"
#include <ESP32Servo.h>

// Pin definitions
#define NUM_LEDS 12  // Increased to handle the 6 additional LEDs
#define DATA_PIN 26

#define SEL0 2
#define SEL1 13
#define SEL2 14
#define SEL3 27
#define IO_IN 32

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)

enum SERVO_MODE {
    FAST,
    SLOW
};

enum BUTTON_PRESSED {
    START_GAME_PRESSED,
    END_GAME_PRESSED,
    RED_PRESSED,
    GREEN_PRESSED,
    AUTO_MODE_PRESSED,
    MANUAL_MODE_PRESSED,
    INDIVIDUAL_MODE_PRESSED,
    TEAM_MODE_PRESSED,
    SENSITIVITY_UP_PRESSED,
    SENSITIVITY_DOWN_PRESSED,
    PLAYER_1_BUTTON_PRESSED,
    PLAYER_2_BUTTON_PRESSED,
    PLAYER_3_BUTTON_PRESSED,
    PLAYER_4_BUTTON_PRESSED,
    PLAYER_5_BUTTON_PRESSED,
    NO_BUTTON_PRESSED
};

// LED Index Definitions
enum LED_INDEX {
    GAME_STATE_LED = 0,
    AUTOMATIC_LED,
    PLAYER1_LED,
    PLAYER2_LED,
    PLAYER3_LED,
    PLAYER4_LED,
    PLAYER5_LED,
    PLAYER6_LED,
    PLAYER7_LED,
    PLAYER8_LED,
    PLAYER9_LED,
    PLAYER10_LED
};

// Sound types
enum SOUND_TYPE {
    RED_LIGHT_SOUND,
    GREEN_LIGHT_SOUND,
    GAME_OVER_SOUND,
    GAME_BEGIN_SOUND,
    ALL_PLAYERS_READY_SOUND
};

#define BRIGHTNESS_SCALE 10

class UI {
public:
    UI();
    void setupPinsAndSensors();
    BUTTON_PRESSED buttonPressed();
    void updateLEDs(GameState gameState, GameMode gameMode, Player players[], int numPlayers);
    void playSound(SOUND_TYPE soundType);
    void printSensitivity(int sensitivity);
    void printMessage(String message);
    void setServo(SERVO_MODE mode);
    void releaseServoFlag();

private:
    Servo myServo;
    const int servoPin = 23;
    const int pulseMin = 500;
    const int pulseMax = 2400;
    bool servoCommandIssued = false;  

    Adafruit_SSD1306 display;
    CRGB leds[NUM_LEDS];
    bool buttonState[10]; // State of each button
    void selectMuxChannel(int channel);
    bool isBusy();
    void setVolume(int volume);
    void executeCMD(byte CMD, byte Par1, byte Par2);
};

#endif // UI_H
