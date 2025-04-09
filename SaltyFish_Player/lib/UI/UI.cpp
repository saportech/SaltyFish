#include "UI.h"

UI::UI() {
    
}

void UI::setup() {

    pinMode(vibrationMotorPin1, OUTPUT);
    pinMode(vibrationMotorPin2, OUTPUT);
    digitalWrite(vibrationMotorPin1, HIGH);
    delay(200);
    digitalWrite(vibrationMotorPin1, LOW);
    digitalWrite(vibrationMotorPin2, LOW);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.show();

    for (int i = 0; i < 5; i++) {
        leds[i] = CRGB::Red;
        FastLED.show();
        delay(100);
        leds[i] = CRGB::Black;
        FastLED.show();
    }

    //setupAudio();

    resetVibrateFlag();
}

void UI::updateReactions(int gameState, int playerStatus) {
    //audio.loop();
    updateLEDs(gameState, playerStatus);  // Update LEDs based on game state and player status

    if (playerStatus == MOVED) {
        vibrateMotor();
    } else if (playerStatus == CROSSED_FINISH_LINE) {
        vibrateMotor();
    }
}

void UI::updateLEDs(int gameState, int playerStatus) {
    static unsigned long lastUpdateMillis = millis();

    // Define colors based on game state and player status
    CRGB gameColor = (gameState == GREEN) ? CRGB::Green : 
                     (gameState == GAME_BEGIN) ? CRGB::Green : 
                     (gameState == RED) ? CRGB::Red : 
                     (gameState == PRE_GAME) ? CRGB::Yellow : 
                     (gameState == GAME_OVER) ? CRGB::Pink : 
                     CRGB::Orange;

    CRGB statusColor = (playerStatus == PLAYING) ? CRGB::Blue : 
                       (playerStatus == NOT_PLAYING) ? CRGB::Red : 
                       (playerStatus == MOVED) ? CRGB::Yellow : 
                       (playerStatus == ESTABLISHED_COMMUNICATION) ? CRGB::Purple : 
                       CRGB::Orange;

    // Update LEDs 0 and 1 with game color
    for (int i = 0; i <= 1; i++) {
        leds[i] = gameColor;
        leds[i].nscale8(ledBrightness);
    }

    // Update LEDs 2 to 5 with status color
    for (int i = 2; i <= 5; i++) {
        leds[i] = statusColor;
        leds[i].nscale8(ledBrightness);
    }

    // Show updates if the time has elapsed
    if (millis() - lastUpdateMillis > 50) {
        lastUpdateMillis = millis();
        FastLED.show();
    }
}

void UI::setupAudio() {
    if (!LittleFS.begin(true)) {
      Serial.println("Failed to mount LittleFS");
      return;
    }
    Serial.println("LittleFS mounted successfully");
  
    listLittleFSFiles();  // List files on LittleFS

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
    while (!SD.begin(SD_CS)) {
      Serial.println("Trying to initialize SD card...");
      delay(1000);
    }
    Serial.println("SD card initialized.");

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(30);
  }

void UI::listLittleFSFiles() {
    Serial.println("Listing files on LittleFS:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("\tSIZE: ");
        Serial.println(file.size());
        file = root.openNextFile();
    }
}

void UI::playSound(SOUND sound) {
    if (audio.isRunning()) return;

    const char* filePath = nullptr;

    switch (sound) {
        case MOVED_SOUND:
            Serial.println("Playing moved sound");
            filePath = "/001.wav";
            break;

        case READY_SOUND:
            Serial.println("Playing ready sound");
            filePath = "/002.wav";
            break;

        case MISSION_ACCOMPLISHED_SOUND:
            Serial.println("Playing mission accomplished sound");
            filePath = "/003.wav";
            break;

        default:
            Serial.println("Error in playSound(): unknown sound enum");
            return;
    }

    if (!audio.connecttoFS(SD, filePath)) {
        Serial.printf("Failed to open audio file: %s\n", filePath);
    } else {
        Serial.printf("Playing audio file: %s\n", filePath);
    }
}

void UI::debugSound() {
    if (!audio.connecttoFS(SD, "/001.wav")) {
        Serial.println("Failed to open audio file");
    } else {
        Serial.println("Playing audio file: 001.mp3");
    }
}

void UI::vibrateMotor() {
    static unsigned long startMillis = millis();
    static unsigned long lastToggleMillis = millis();
    static bool motorOn = false;

    if (!motorActivated) {
        motorActivated = true;
        startMillis = millis();
        lastToggleMillis = millis();
        motorOn = true;
        digitalWrite(vibrationMotorPin1, HIGH);  // Start vibration
    }

    if (motorActivated) {
        unsigned long currentMillis = millis();

        // if (currentMillis - lastToggleMillis >= 200) {
        //     lastToggleMillis = currentMillis;
        //     motorOn = !motorOn;
        //     digitalWrite(vibrationMotorPin1, motorOn ? HIGH : LOW);  // Toggle motor state
        // }

        if (currentMillis - startMillis >= 2000) {
            digitalWrite(vibrationMotorPin1, LOW);
            //motorOn = false;
        }
    }
}

void UI::resetVibrateFlag() {
    motorActivated = false;
}

void UI::setBrightness(uint8_t brightness) {
    ledBrightness = brightness;
}
