#include <Arduino.h>
#include <vector>
#include "Game.h"
#include "Com.h"
#include "Player.h"
#include "UI.h"
#include <deque>

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

class SoundQueue {
    private:
        std::deque<SOUND_TYPE> queue;
        unsigned long lastSoundTime = 0;
        const unsigned long soundDuration = 3000;
    
    public:
        void enqueue(SOUND_TYPE soundID) {
            queue.push_back(soundID);
        }
        
        void enqueuePriority(SOUND_TYPE soundID) {
            queue.push_front(soundID);
        }
        
        void update(UI& ui) {
            unsigned long now = millis();
            if (now - lastSoundTime >= soundDuration) {
                if (!queue.empty()) {
                    SOUND_TYPE nextSound = queue.front();
                    queue.pop_front();
                    ui.playSound(nextSound);
                    lastSoundTime = now;
                }
            }
        }
    };

SoundQueue soundQueue;

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

    soundQueue.update(ui);

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
        ui.updateLEDs(game.getState(),game.getGameMode(), players, NUM_PLAYERS, game.getSensitivity());   
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
                nextChangeMillis = getRandomTime(6000, 15000); // Set a new random time
            } else if (state == WAIT_FOR_MOVEMENT_DETECTION_DURING_RED_LIGHT) {
                handleGameState(GREEN);
                previousMillisGreenDelay = millis();
                state = GREEN_LIGHT_DELAY;
                nextChangeMillis = getRandomTime(4000, 12000); // Set a new random time
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
                //ui.playSound(ALL_PLAYERS_READY_SOUND);
                soundQueue.enqueuePriority(ALL_PLAYERS_READY_SOUND);
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
                    if (players[i].getId() == 1) {
                        soundQueue.enqueue(PLAYER_1_FINISH_SOUND);
                    } else if (players[i].getId() == 2) {
                        soundQueue.enqueue(PLAYER_2_FINISH_SOUND);
                    } else if (players[i].getId() == 3) {
                        soundQueue.enqueue(PLAYER_3_FINISH_SOUND);
                    } else if (players[i].getId() == 4) {
                        soundQueue.enqueue(PLAYER_4_FINISH_SOUND);
                    } else if (players[i].getId() == 5) {
                        soundQueue.enqueue(PLAYER_5_FINISH_SOUND);
                    } else if (players[i].getId() == 6) {
                        soundQueue.enqueue(PLAYER_6_FINISH_SOUND);
                    } else if (players[i].getId() == 7) {
                        soundQueue.enqueue(PLAYER_7_FINISH_SOUND);
                    } else if (players[i].getId() == 8) {
                        soundQueue.enqueue(PLAYER_8_FINISH_SOUND);
                    } else if (players[i].getId() == 9) {
                        soundQueue.enqueue(PLAYER_9_FINISH_SOUND);
                    } else if (players[i].getId() == 10) {
                        soundQueue.enqueue(PLAYER_10_FINISH_SOUND);
                    }
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
                    if (players[i].getId() == 1) {
                        soundQueue.enqueue(PLAYER_1_MOVED_SOUND);
                    } else if (players[i].getId() == 2) {
                        soundQueue.enqueue(PLAYER_2_MOVED_SOUND);
                    } else if (players[i].getId() == 3) {
                        soundQueue.enqueue(PLAYER_3_MOVED_SOUND);
                    } else if (players[i].getId() == 4) {
                        soundQueue.enqueue(PLAYER_4_MOVED_SOUND);
                    } else if (players[i].getId() == 5) {
                        soundQueue.enqueue(PLAYER_5_MOVED_SOUND);
                    } else if (players[i].getId() == 6) {
                        soundQueue.enqueue(PLAYER_6_MOVED_SOUND);
                    } else if (players[i].getId() == 7) {
                        soundQueue.enqueue(PLAYER_7_MOVED_SOUND);
                    } else if (players[i].getId() == 8) {
                        soundQueue.enqueue(PLAYER_8_MOVED_SOUND);
                    } else if (players[i].getId() == 9) {
                        soundQueue.enqueue(PLAYER_9_MOVED_SOUND);
                    } else if (players[i].getId() == 10) {
                        soundQueue.enqueue(PLAYER_10_MOVED_SOUND);
                    }
                    players[i].setStatus(NOT_PLAYING);
                }
            }
            comm.resetMsg();
            break;
        case GREEN_LIGHT_DELAY:
            if (millis() - previousMillisGreenDelay >= 100) {
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
            soundQueue.enqueuePriority(GAME_BEGIN_SOUND);
            ui.setServo(SERVO_MODE::SERVO_GREEN);
            //ui.playSound(GAME_BEGIN_SOUND);
            break;
        case RED:
            Serial.print("Red light");
            soundQueue.enqueuePriority(RED_LIGHT_SOUND);
            //ui.playSound(RED_LIGHT_SOUND);
            ui.setServo(SERVO_MODE::SERVO_RED);
            break;
        case GREEN:
            Serial.print("Green light");
            soundQueue.enqueuePriority(GREEN_LIGHT_SOUND);
            //ui.playSound(GREEN_LIGHT_SOUND);
            ui.setServo(SERVO_MODE::SERVO_GREEN);
            break;
        case GAME_OVER:
            Serial.print("Game over");
            soundQueue.enqueuePriority(GAME_OVER_SOUND);
            //ui.playSound(GAME_OVER_SOUND);
            break;
    }

    Serial.println(" Sensitivity: " + String(game.getSensitivity()));
}

void uiUpdate() {
    static unsigned long lastButtonPressMillis = 0;
    const unsigned long buttonCooldown = 200;

    pressedButton = ui.buttonPressed();

    if (millis() - lastButtonPressMillis >= buttonCooldown) {
        if (pressedButton == AUTO_MODE_PRESSED) {
            if (game.getGameMode() == INDIVIDUAL_AUTOMATIC) {
                game.setGameMode(INDIVIDUAL_MANUAL);
            } else {
                game.setGameMode(INDIVIDUAL_AUTOMATIC);
            }
            GameMode currentMode = game.getGameMode();
            Serial.print("Game mode: ");
            Serial.println(currentMode == INDIVIDUAL_AUTOMATIC ? "INDIVIDUAL_AUTOMATIC" : "INDIVIDUAL_MANUAL");
            lastButtonPressMillis = millis();
        } else if (pressedButton == SENSITIVITY_CHANGE_PRESSED) {
            game.setSensitivity(game.getSensitivity() + 1);//if larger than 3, set to 1
            Serial.print("Sensitivity: ");
            Serial.println(game.getSensitivity());

            lastButtonPressMillis = millis();
        }
    }

    if (game.getState() == GameState::RED) {
        ui.setServo(SERVO_MODE::SERVO_SPECIAL);
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

    ui.updateLEDs(game.getState(), game.getGameMode(), players, NUM_PLAYERS, game.getSensitivity());
    
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
