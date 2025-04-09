#include "UI.h"

UI::UI() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {

}

void UI::setServo(SERVO_MODE mode) {
    if (!servoCommandIssued) {
        if (mode == FAST) {
        myServo.writeMicroseconds(pulseMin);
        } else if (mode == SLOW) {
        myServo.writeMicroseconds(pulseMax);
        }
        servoCommandIssued = true;
    }
}

void UI::releaseServoFlag() {
    servoCommandIssued = false;
}

void UI::setupPinsAndSensors() {
    myServo.setPeriodHertz(50);
    myServo.attach(servoPin, pulseMin, pulseMax);
    myServo.writeMicroseconds(pulseMin);    
    
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.show();

    //show leds for testing
    for (int i = 0; i < 3; i++) {
        leds[i] = CRGB::Red;
        FastLED.show();
        delay(100);
        leds[i] = CRGB::Black;
        FastLED.show();
    }

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is the I2C address for the OLED
        Serial.println(F("SSD1306 allocation failed"));
    }

    display.clearDisplay();
    display.display();

    pinMode(SEL0, OUTPUT);
    pinMode(SEL1, OUTPUT);
    pinMode(SEL2, OUTPUT);
    pinMode(SEL3, OUTPUT);
    pinMode(IO_IN, INPUT_PULLUP);

    Serial2.begin(9600, SERIAL_8N1, 34, 12);
    setVolume(22);



}

BUTTON_PRESSED UI::buttonPressed() {
    static int currentChannel = 0;
    static unsigned long lastMillis = 0;

    // Select the current MUX channel
    selectMuxChannel(currentChannel);

    // Check if 1 millisecond has passed
    if (millis() - lastMillis >= 1) {
        lastMillis = millis();  // Update the lastMillis for the next cycle

        // Read the button state after the delay
        int buttonState = digitalRead(IO_IN);
        if (buttonState == LOW) {
            BUTTON_PRESSED button = static_cast<BUTTON_PRESSED>(currentChannel);
            currentChannel = (currentChannel + 1) % 15;  // Move to the next channel for the next cycle
            //Serial.println("Button pressed: " + String(button));
            return button;
        }

        // Move to the next channel
        currentChannel = (currentChannel + 1) % 15;
    }

    // No button pressed, return NO_BUTTON_PRESSED
    return NO_BUTTON_PRESSED;
}

void UI::selectMuxChannel(int channel) {
    digitalWrite(SEL0, (channel & 0x01) ? HIGH : LOW);
    digitalWrite(SEL1, (channel & 0x02) ? HIGH : LOW);
    digitalWrite(SEL2, (channel & 0x04) ? HIGH : LOW);
    digitalWrite(SEL3, (channel & 0x08) ? HIGH : LOW);
}

void UI::updateLEDs(GameState gameState, GameMode gameMode, Player players[], int numPlayers) {
    static unsigned long lastUpdateMillis = millis();

    // Update player LEDs
    for (int i = 0; i < numPlayers; i++) {

        if (gameState == GAME_OVER) {
            leds[i + 2] = CRGB::Yellow; // Turn all player LEDs yellow if the game is over
        } else {
            switch (players[i].getStatus()) {
                case PLAYING:
                    leds[i + 2] = CRGB::Blue;
                    break;
                case NOT_PLAYING:
                    leds[i + 2] = CRGB::Red;
                    break;
                case IDLE:
                    leds[i + 2] = CRGB::Yellow;
                    break;
                case ESTABLISHED_COMMUNICATION:
                    leds[i + 2] = CRGB::Purple;
                    break;
                default:
                    leds[i + 2] = CRGB::Orange; // Default color if none of the conditions are met
                    break;
            }
        }
        leds[i + 2].nscale8(BRIGHTNESS_SCALE);  // Apply brightness scale
    }

    // Update game state LEDs
    CRGB gameStateColor;
    switch (gameState) {
        case GAME_BEGIN:
            gameStateColor = CRGB::Green;
            break;
        case GREEN:
            gameStateColor = CRGB::Green;
            break;
        case RED:
            gameStateColor = CRGB::Red;
            break;
        case GAME_OVER:
            gameStateColor = CRGB::Yellow;
            break;
        default:
            gameStateColor = CRGB::Orange;
            break;
    }

    leds[GAME_STATE_LED] = gameStateColor;
    leds[GAME_STATE_LED].nscale8(BRIGHTNESS_SCALE); 
    
    // Update game mode LEDs
    switch (gameMode) {
        case INDIVIDUAL_AUTOMATIC:
            leds[AUTOMATIC_LED] = CRGB::Green;
            //leds[MANUAL_LED] = CRGB::Black;
            break;
        case INDIVIDUAL_MANUAL:
            leds[AUTOMATIC_LED] = CRGB::Black;
            //leds[MANUAL_LED] = CRGB::Green;
            break;
        default:
            leds[AUTOMATIC_LED] = CRGB::Orange;
            //leds[MANUAL_LED] = CRGB::Orange;
            break;
    }
    leds[AUTOMATIC_LED].nscale8(BRIGHTNESS_SCALE);
    //leds[MANUAL_LED].nscale8(BRIGHTNESS_SCALE);

    if (millis() - lastUpdateMillis > 50) {
        lastUpdateMillis = millis();
        FastLED.show();
    }
}

void UI::playSound(SOUND_TYPE soundType) {
    Serial.println("Playing file number: " + String(soundType));

    switch (soundType) {
        case RED_LIGHT_SOUND:
            executeCMD(0x0F, 0x01, 0x01);
            break;
        case GREEN_LIGHT_SOUND:
            executeCMD(0x0F, 0x01, 0x02);
            break;
        case GAME_OVER_SOUND:
            executeCMD(0x0F, 0x01, 0x03);
            break;
        case GAME_BEGIN_SOUND:
            executeCMD(0x0F, 0x01, 0x04);
            break;
        case ALL_PLAYERS_READY_SOUND:
            executeCMD(0x0F, 0x01, 0x05);
            break;
        default:
            Serial.println("Error in playSound(): Unknown sound type");
            break;
    }
}

void UI::executeCMD(byte CMD, byte Par1, byte Par2) {
    #define Start_Byte 0x7E
    #define Version_Byte 0xFF
    #define Command_Length 0x06
    #define End_Byte 0xEF
    #define Acknowledge 0x00 //Returns info with command 0x41 [0x01: info, 0x00: no info]
    
    // Calculate the checksum (2 bytes)
    word checksum = -(Version_Byte + Command_Length + CMD + Acknowledge + Par1 + Par2);
    
    // Build the command line
    byte Command_line[10] = { Start_Byte, Version_Byte, Command_Length, CMD, Acknowledge,
                                Par1, Par2, highByte(checksum), lowByte(checksum), End_Byte };
    
    // Send the command line to the module
    for (byte k = 0; k < 10; k++) {
        Serial2.write(Command_line[k]);
    }
}

bool UI::isBusy() {
    #define BUSY_PIN 25
    pinMode(BUSY_PIN, INPUT);
    int busyRead = digitalRead(BUSY_PIN);
    if (busyRead == 1) {
        //Serial.println("DFPlayer not busy!");
        return false;
    }
    return true;
}

void UI::setVolume(int volume) {
    executeCMD(0x06, 0, volume);
    delay(100);
    Serial.println("Volume set to: " + String(volume));
}

void UI::printSensitivity(int sensitivity) {
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 15);             // Start at top-left corner
    display.print("LEVEL:");
    display.setCursor(60, 45);            // Move to next line
    display.print(sensitivity);
    display.display();
}

void UI::printMessage(String message) {
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 15);             // Start at top-left corner
    display.print(F("Revive"));
    display.setCursor(30, 45);            // Move to next line
    display.print("Player?");
    display.display();
}