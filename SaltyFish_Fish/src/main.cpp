#include <Arduino.h>
#include <vector>
#include "Game.h"
#include "Com.h"
#include "Player.h"
#include "UI.h"

#define NUM_PLAYERS 10
std::vector<int> playerIDs(NUM_PLAYERS);

Com comm;
Player players[NUM_PLAYERS];
Game game;
UI ui;
BUTTON_PRESSED pressedButton;

void brainStateMachine();
void sendMessageToAllPlayers(GameState state);
void handleGameState(GameState newGameState);
void uiUpdate();
void resetValues(int resetType);
unsigned long getRandomTime(unsigned long minTime, unsigned long maxTime);
const char* getStateName(int state);
const char* getGameStateName(GameState gameState);
void loopAnalysis();

void setup() {

    Serial.begin(115200);
    Serial.println("Red Light Green Light Brain Unit");

    game.begin();

    ui.setupPinsAndSensors();

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        playerIDs[i] = i + 1;
    }

    for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i].begin(playerIDs[i]);
        Serial.print("Initialized player with ID: ");
        Serial.println(players[i].getId());
        players[i].setStatus(IDLE);
    }

    comm.begin();

    resetValues(PRE_GAME);
}

void loop() {

    uiUpdate();

    brainStateMachine();
    
}

void brainStateMachine() {
    static int state = 0;
    static unsigned long previousMillis = 0;
    static unsigned long nextChangeMillis = 0;
    static unsigned long previousMillisGreenDelay = 0;
    int establishRes = 0;

    Com::Msg message;

    enum RED_GREEN_STATE_TYPE { 
        CHECK_COMMUNICATION,
        START,
        GREEN_LIGHT, 
        WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT,
        GREEN_LIGHT_DELAY,
        STATE_GAMEOVER
    };

    static unsigned long lastPrintMillis = 0;
    if (millis() - lastPrintMillis >= 2000) {
        Serial.print("Brain State: ");
        Serial.print(getStateName(state));
        Serial.print(", Game State: ");
        Serial.print(getGameStateName(game.getState()));
        Serial.print(" Game Mode: ");
        Serial.print(game.getGameMode() == INDIVIDUAL_AUTOMATIC ? "INDIVIDUAL_AUTOMATIC" : "INDIVIDUAL_MANUAL");
        Serial.print(" Sensitivity: ");
        Serial.println(game.getSensitivity());
        lastPrintMillis = millis();
    }

    comm.receiveData();
    sendMessageToAllPlayers(game.getState());

    if (state != GREEN_LIGHT_DELAY) {
        ui.updateLEDs(game.getState(),game.getGameMode(), players, NUM_PLAYERS);    
    }

    if (pressedButton == END_GAME_PRESSED) {
        handleGameState(GAME_OVER);
        delay(400);
        state = STATE_GAMEOVER;
    }

    if (game.getGameMode() == INDIVIDUAL_AUTOMATIC) {
        if (millis() - previousMillis >= nextChangeMillis) {
            if (state == GREEN_LIGHT || (state == START && game.getState() == GAME_BEGIN)) {
                comm.resetMsg();
                handleGameState(RED);
                state = WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT;
                nextChangeMillis = getRandomTime(4000, 10000); // Set a new random time
            } else if (state == WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT) {
                handleGameState(GREEN);
                previousMillisGreenDelay = millis();
                state = GREEN_LIGHT_DELAY;
                nextChangeMillis = getRandomTime(8000, 12000); // Set a new random time
            }
            previousMillis = millis();
        }
    }
    
    switch (state) {
        case CHECK_COMMUNICATION:
            message = comm.getMsg();
            establishRes = comm.establishedCommunication(message, playerIDs);
            if (establishRes != 0) {
                players[establishRes - 1].setStatus(ESTABLISHED_COMMUNICATION);
            }
            if (establishRes == comm.brainId) {
                Serial.println("Communication established with all players");
                ui.playSound(ALL_PLAYERS_READY_SOUND);
                state = START;
            }
            comm.resetMsg();
            if (pressedButton == START_GAME_PRESSED && game.getState() != GAME_BEGIN) {
                state = START;
            }
            break;
        case START:
            if (pressedButton == START_GAME_PRESSED && game.getState() != GAME_BEGIN) {
                for (int i = 0; i < NUM_PLAYERS; i++) {
                    players[i].setStatus(PLAYING);
                }
                handleGameState(GAME_BEGIN);
            }
            if (game.getState() == GAME_BEGIN) {
                if (pressedButton == RED_PRESSED) {
                    comm.resetMsg();
                    handleGameState(RED);
                    state = WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT;
                }
            }
            break;
        case GREEN_LIGHT: // State is GREEN now
            message = comm.getMsg();
            if (pressedButton == RED_PRESSED) {
                comm.resetMsg();
                handleGameState(RED);
                state = WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT;
            }
            for (int i = 0; i < NUM_PLAYERS; i++) {
                if (message.id_sender == players[i].getId() && message.player_status == CROSSED_FINISH_LINE && players[i].getStatus() == PLAYING) {
                    Serial.println("Player " + String(players[i].getId()) + " crossed finish line");
                    players[i].setStatus(CROSSED_FINISH_LINE);
                }
            }
            comm.resetMsg();
            break;
        case WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT: // State is RED now
            message = comm.getMsg();
            if (pressedButton == GREEN_PRESSED) {
                handleGameState(GREEN);
                previousMillisGreenDelay = millis();
                state = GREEN_LIGHT_DELAY;
            }
            for (int i = 0; i < NUM_PLAYERS; i++) {
                if (message.id_sender == players[i].getId() && message.player_status == MOVED && players[i].getStatus() == PLAYING) {
                    Serial.println("Player " + String(players[i].getId()) + " moved during red light");
                    players[i].setStatus(NOT_PLAYING);
                }
            }
            comm.resetMsg();
            break;
        case GREEN_LIGHT_DELAY:
            if (millis() - previousMillisGreenDelay >= 2000) {
                state = GREEN_LIGHT;
            }
            break;
        case STATE_GAMEOVER:
            resetValues(GAME_OVER);
            state = START;
            break;
    }
}

void sendMessageToAllPlayers(GameState state) {
    static unsigned long previousMillis = millis();
    static unsigned long previousMillisGeneral = 0;
    static int currentPlayerIndex = 0;
    static unsigned long sendPlayerMillis = millis();

    #define SEND_INTERVAL 100
    #define PLAYER_SEND_INTERVAL (SEND_INTERVAL / NUM_PLAYERS)

    if (millis() - sendPlayerMillis > PLAYER_SEND_INTERVAL) {
        comm.sendMessage(comm.brainId, players[currentPlayerIndex].getId(), game.getSensitivity(), state, players[currentPlayerIndex].getStatus());

        currentPlayerIndex++;
        if (currentPlayerIndex >= NUM_PLAYERS) {
            currentPlayerIndex = 0;
            previousMillis = millis();
        }

        sendPlayerMillis = millis();
    }
}

void handleGameState(GameState newGameState) {
    game.setState(newGameState);
    ui.releaseServoFlag();

    switch (game.getState()) {
        case GAME_BEGIN:
            Serial.print("Game begin");
            ui.playSound(GAME_BEGIN_SOUND);
            break;
        case RED:
            Serial.print("Red light");
            ui.playSound(RED_LIGHT_SOUND);
            ui.setServo(SERVO_MODE::FAST);
            break;
        case GREEN:
            Serial.print("Green light");
            ui.playSound(GREEN_LIGHT_SOUND);
            ui.setServo(SERVO_MODE::SLOW);
            break;
        case GAME_OVER:
            Serial.print("Game over");
            ui.playSound(GAME_OVER_SOUND);
            break;
    }

    Serial.println(" Sensitivity: " + String(game.getSensitivity()));
}

void uiUpdate() {
    static unsigned long lastButtonPressMillis = 0;
    static unsigned long firstPressMillis = 0;
    static BUTTON_PRESSED lastPressedButton = NO_BUTTON_PRESSED;
    const unsigned long buttonCooldown = 200;
    const unsigned long doublePressWindow = 5000; // 5 seconds

    pressedButton = ui.buttonPressed();

    if (millis() - lastButtonPressMillis >= buttonCooldown) {
        if (pressedButton == AUTO_MODE_PRESSED) {
            game.setGameMode(INDIVIDUAL_AUTOMATIC);
            Serial.println("Individual automatic mode");
            lastButtonPressMillis = millis();
        } else if (pressedButton == MANUAL_MODE_PRESSED) {
            game.setGameMode(INDIVIDUAL_MANUAL);
            Serial.println("Individual manual mode");
            lastButtonPressMillis = millis();
        } else if (pressedButton == SENSITIVITY_UP_PRESSED) {
            game.setSensitivity(game.getSensitivity() + 1);
            ui.printSensitivity(game.getSensitivity());
            lastButtonPressMillis = millis();
        } else if (pressedButton == SENSITIVITY_DOWN_PRESSED) {
            game.setSensitivity(game.getSensitivity() - 1);
            ui.printSensitivity(game.getSensitivity());
            lastButtonPressMillis = millis();
        } else if (pressedButton >= PLAYER_1_BUTTON_PRESSED && pressedButton <= PLAYER_5_BUTTON_PRESSED) {
            int playerIndex = pressedButton - PLAYER_1_BUTTON_PRESSED;
            if (playerIndex < NUM_PLAYERS) {
                if (lastPressedButton == pressedButton && millis() - firstPressMillis <= doublePressWindow) {
                    // Second press detected within 5 seconds
                    Serial.println("Second press detected for Player " + String(playerIndex + 1) + ". Performing second action.");
                    players[playerIndex].setStatus(PLAYING);
                    ui.printSensitivity(game.getSensitivity());
                    lastButtonPressMillis = millis();
                    lastPressedButton = NO_BUTTON_PRESSED; // Reset after the second action
                } else {
                    // First press detected
                    Serial.println("First press detected for Player " + String(playerIndex + 1) + ". Performing first action.");
                    ui.printMessage("Revive\nPlayer?");
                    //ui.printSensitivity(playerIndex + 1);  // Example first action, replace with your actual function
                    firstPressMillis = millis();
                    lastPressedButton = pressedButton;
                    lastButtonPressMillis = millis();
                }
            }
        }
    }

    // If 5 seconds have passed without a second press, reset the state
    if (lastPressedButton != NO_BUTTON_PRESSED && millis() - firstPressMillis > doublePressWindow) {
        ui.printSensitivity(game.getSensitivity());
        lastPressedButton = NO_BUTTON_PRESSED;
    }


}

void resetValues(int resetType) {
    
    game.setGameMode(INDIVIDUAL_MANUAL);
    ui.releaseServoFlag();

    
    if (resetType == PRE_GAME) {
        game.setState(PRE_GAME);

        for (int i = 0; i < NUM_PLAYERS; i++) {
            players[i].setStatus(IDLE);
        }
    }
    else if (resetType == GAME_OVER) {
        game.setState(GAME_OVER);

        for (int i = 0; i < NUM_PLAYERS; i++) {
            players[i].setStatus(NOT_PLAYING);
        }
    }

    comm.resetMsg();

    ui.updateLEDs(game.getState(), game.getGameMode(), players, NUM_PLAYERS);

    ui.printSensitivity(game.getSensitivity());
    
}

unsigned long getRandomTime(unsigned long minTime, unsigned long maxTime) {
    return random(minTime, maxTime);
}

const char* getStateName(int state) {
    switch (state) {
        case 0: return "CHECK_COMMUNICATION";
        case 1: return "START";
        case 2: return "GREEN_LIGHT";
        case 3: return "WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT";
        case 4: return "GREEN_LIGHT_DELAY";
        case 5: return "STATE_GAMEOVER";
        default: return "UNKNOWN_STATE";
    }
}

const char* getGameStateName(GameState gameState) {
    switch (gameState) {
        case PRE_GAME: return "PRE_GAME";
        case GAME_BEGIN: return "GAME_BEGIN";
        case RED: return "RED";
        case GREEN: return "GREEN";
        case GAME_OVER: return "GAME_OVER";
        default: return "UNKNOWN_GAME_STATE";
    }
}

void loopAnalysis()
{
  static unsigned long previousMillis = 0;
  static unsigned long lastMillis = 0;
  static unsigned long minLoopTime = 0xFFFFFFFF;
  static unsigned long maxLoopTime = 0;
  static unsigned long loopCounter = 0;

  #define INTERVAL 1000

  unsigned long currentMillis = millis();
  if ( currentMillis - previousMillis > INTERVAL )
  {
    Serial.print( "Loops: " );
    Serial.print( loopCounter );
    Serial.print( " ( " );
    Serial.print( minLoopTime );
    Serial.print( " / " );
    Serial.print( maxLoopTime );
    Serial.println( " )" );
    previousMillis = currentMillis;
    loopCounter = 0;
    minLoopTime = 0xFFFFFFFF;
    maxLoopTime = 0;
  }
  loopCounter++;
  unsigned long loopTime = currentMillis - lastMillis;
  lastMillis = currentMillis;
  if ( loopTime < minLoopTime )
  {
    minLoopTime = loopTime;
  }
  if ( loopTime > maxLoopTime )
  {
    maxLoopTime = loopTime;
  }

}
