#include "UI.h"

void UI::setServo(SERVO_MODE mode) {
    static unsigned long lastToggleTime = 0;
    static unsigned long currentDelay = 0;

    if (mode == SERVO_SPECIAL) {
        unsigned long now = millis();
        if (now - lastToggleTime >= currentDelay) {
            lastToggleTime = now;
            currentDelay = random(200, 1001); // random delay between 200ms and 1000ms

            int halfway = (pulseMin + pulseMax) / 2;
            int randomPulse = random(pulseMin, halfway + 1);  // random position between pulseMin and halfway
            myServo.writeMicroseconds(randomPulse);
        }
        return;
    }

    if (!servoCommandIssued) {
        if (mode == SERVO_RED) {
            myServo.writeMicroseconds(pulseMin);
        } else if (mode == SERVO_GREEN) {
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

void UI::updateLEDs(GameState gameState, GameMode gameMode, Player players[], int numPlayers, int sensitivity) {
    static unsigned long lastUpdateMillis = millis();

    // Update sensitivity LED
    switch (sensitivity)
    {
    case 1:
        leds[SENSITIVITY_LED] = CRGB::Green;
        break;
    case 2:
        leds[SENSITIVITY_LED] = CRGB::Purple;
        break;
    case 3:
        leds[SENSITIVITY_LED] = CRGB::Red;
        break;
    default:
        break;
    }

    leds[SENSITIVITY_LED].nscale8(BRIGHTNESS_SCALE);

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
    
    leds[GAME_STATE_LED_2] = gameStateColor;
    leds[GAME_STATE_LED_2].nscale8(BRIGHTNESS_SCALE);

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
        case PLAYER_1_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x06);
            break;
        case PLAYER_2_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x07);
            break;
        case PLAYER_3_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x08);
            break;
        case PLAYER_4_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x09);
            break;
        case PLAYER_5_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x0A);
            break;
        case PLAYER_6_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x0B);
            break;
        case PLAYER_7_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x0C);
            break;
        case PLAYER_8_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x0D);
            break;
        case PLAYER_9_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x0E);
            break;
        case PLAYER_10_MOVED_SOUND:
            executeCMD(0x0F, 0x01, 0x0F);
            break;
        case PLAYER_1_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x10);
            break;
        case PLAYER_2_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x11);
            break;
        case PLAYER_3_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x12);
            break;
        case PLAYER_4_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x13);
            break;
        case PLAYER_5_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x14);
            break;
        case PLAYER_6_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x15);
            break;
        case PLAYER_7_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x16);
            break;
        case PLAYER_8_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x17);
            break;
        case PLAYER_9_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x18);
            break;
        case PLAYER_10_FINISH_SOUND:
            executeCMD(0x0F, 0x01, 0x19);
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

