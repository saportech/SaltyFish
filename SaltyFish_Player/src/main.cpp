#include <Arduino.h>
#include "Game.h"
#include "Com.h"
#include "Player.h"
#include "UI.h"

//#define DEBUG 1

Game game;
Com comm;
Player player;
UI ui;

int playerId;

void playerStateMachine();
void handleGamePlayerState(GameState newGameState, PlayerStatus newPlayerStatus, int newSensitivity);
void loopAnalysis();

void setup() {
    Serial.begin(115200);
    #ifdef DEBUG
        Serial.println("Red Light Green Light Player Unit");
    #endif

    ui.setup();

    player.begin();
    playerId = player.getId();

    comm.begin(playerId);
    
}

void loop() {

    playerStateMachine();

}

void playerStateMachine() {
    static int state = 0;
    static bool gameOverExecuted = false;
    static unsigned long previousMillis = 0;
    static unsigned long previousSendMillis = 0;
    static unsigned long previousPrintMillis = 0;

    enum RED_GREEN_STATE_TYPE {
        COMMUNICATION_SETUP,
        START,
        GREEN_LIGHT,
        WAIT_BEFORE_RED_LIGHT,
        CHECK_IF_MOVED_DURING_RED_LIGHT,
        NOTIFY_BRAIN_OF_GAME_OVER,
        STATE_CELEBRATE_VICTORY_OR_LOSS,
        STATE_GAMEOVER
    };

    Com::Msg message;
    comm.receiveData();
    message = comm.getMsg();
    ui.updateReactions(game.getState(), player.getStatus());
    game.setSensitivity(message.sensitivity);

    if (millis() - previousPrintMillis > 3000) {
        previousPrintMillis = millis();
        #ifdef DEBUG
        Serial.print("Player ID: " + String(playerId));
        Serial.print(" State: ");
        switch (state) {
            case COMMUNICATION_SETUP: Serial.print("COMMUNICATION_SETUP"); break;
            case START: Serial.print("START"); break;
            case GREEN_LIGHT: Serial.print("GREEN_LIGHT"); break;
            case WAIT_BEFORE_RED_LIGHT: Serial.print("WAIT_BEFORE_RED_LIGHT"); break;
            case CHECK_IF_MOVED_DURING_RED_LIGHT: Serial.print("CHECK_IF_MOVED_DURING_RED_LIGHT"); break;
            case NOTIFY_BRAIN_OF_GAME_OVER: Serial.print("NOTIFY_BRAIN_OF_GAME_OVER"); break;
            case STATE_CELEBRATE_VICTORY_OR_LOSS: Serial.print("STATE_CELEBRATE_VICTORY_OR_LOSS"); break;
            case STATE_GAMEOVER: Serial.print("STATE_GAMEOVER"); break;
            default: Serial.print("UNKNOWN_STATE"); break;
        }
        Serial.print(" Message: " + String(message.id_sender) + " " + String(message.id_receiver) + " " + String(comm.gameStateToString(message.game_state)) + " " + String(comm.playerStatusToString((message.player_status))));
        Serial.print(" Game state: " + String(comm.gameStateToString(game.getState())));
        Serial.print(" Player status: " + String(comm.playerStatusToString(player.getStatus())));
        Serial.println(" Sensitivity: " + String(message.sensitivity));

        #endif
    }

    if (message.game_state == GAME_OVER) {
        state = STATE_GAMEOVER;
    }

    switch (state) {
        case COMMUNICATION_SETUP:
            if (comm.establishedCommunication(message, playerId) ||
                (message.player_status == PLAYING && player.getStatus() != PLAYING ) ) {
                handleGamePlayerState(PRE_GAME, ESTABLISHED_COMMUNICATION, message.sensitivity);
                //ui.playSound(READY_SOUND);
                #ifdef DEBUG
                Serial.println("Communication established");
                #endif
                state = START;
            }
            break;
        case START://State will be GREEN after GAME_BEGIN
            if (message.game_state == GAME_BEGIN && game.getState() != GAME_BEGIN || 
                (message.player_status == PLAYING && player.getStatus() != PLAYING)) {
                handleGamePlayerState(GAME_BEGIN, PLAYING, message.sensitivity);
                gameOverExecuted = false;
                break;
            }
            if (game.getState() == GAME_BEGIN) {
                if (comm.checkBrainNearby() && player.getStatus() == PLAYING) {
                    Serial.println("Player " + String(playerId) + " crossed the finish line");
                    //ui.playSound(MISSION_ACCOMPLISHED_SOUND);
                    handleGamePlayerState(GREEN, CROSSED_FINISH_LINE, message.sensitivity);
                    comm.sendMessage(playerId, 9, game.getSensitivity(), game.getState(), player.getStatus());
                    ui.resetVibrateFlag();
                    previousMillis = millis();
                    state = NOTIFY_BRAIN_OF_GAME_OVER;
                }
                else if (message.game_state == RED) {
                    handleGamePlayerState(RED, player.getStatus(), message.sensitivity);
                    previousMillis = millis();
                    state = WAIT_BEFORE_RED_LIGHT;
                }
            }
            break;
        case GREEN_LIGHT://State is GREEN
            if (message.game_state == RED) {
                handleGamePlayerState(RED, player.getStatus(), message.sensitivity);
                previousMillis = millis();
                state = WAIT_BEFORE_RED_LIGHT;
            } else if (comm.checkBrainNearby() && player.getStatus() == PLAYING) {
                Serial.println("Player " + String(playerId) + " crossed the finish line");
                //ui.playSound(MISSION_ACCOMPLISHED_SOUND);
                handleGamePlayerState(GREEN, CROSSED_FINISH_LINE, message.sensitivity);
                comm.sendMessage(playerId, 9, game.getSensitivity(), game.getState(), player.getStatus());
                ui.resetVibrateFlag();
                previousMillis = millis();
                state = NOTIFY_BRAIN_OF_GAME_OVER;
            }
            break;
        case WAIT_BEFORE_RED_LIGHT://State is before RED
            if (millis() - previousMillis >= 4000) {
                state = CHECK_IF_MOVED_DURING_RED_LIGHT;
            }
            break;
        case CHECK_IF_MOVED_DURING_RED_LIGHT://State is RED
            if (player.movedDuringRedLight(game.getSensitivity()) && player.getStatus() == PLAYING) {
                ui.resetVibrateFlag();
                //ui.playSound(MOVED_SOUND);
                handleGamePlayerState(GAME_OVER, MOVED, message.sensitivity);
                comm.sendMessage(playerId, 9, game.getSensitivity(), game.getState(), player.getStatus());
                Serial.println("Player " + String(playerId) + " moved during red light");
                previousMillis = millis();
                previousSendMillis = millis();
                state = NOTIFY_BRAIN_OF_GAME_OVER;
            }
            if (message.game_state == GREEN) {
                handleGamePlayerState(GREEN, player.getStatus(), message.sensitivity);
                state = GREEN_LIGHT;
            }
            break;
        case NOTIFY_BRAIN_OF_GAME_OVER:
            if (millis() - previousSendMillis > 500) {
                comm.sendMessage(playerId, 9, game.getSensitivity(), game.getState(), player.getStatus());
                previousSendMillis = millis();
            }
            if (message.id_receiver == playerId && (message.player_status == NOT_PLAYING || message.player_status == CROSSED_FINISH_LINE)) {
                Serial.println("Player " + String(playerId) + " got the ACK from the brain");
                previousSendMillis = millis();
                state = STATE_CELEBRATE_VICTORY_OR_LOSS;
            }
            break;
        case STATE_CELEBRATE_VICTORY_OR_LOSS:
            if (millis() - previousMillis > 4000) {
                state = STATE_GAMEOVER;
            }
            break;
        case STATE_GAMEOVER:
            if (!gameOverExecuted) {
                handleGamePlayerState(GAME_OVER, NOT_PLAYING, message.sensitivity);
                gameOverExecuted = true;
            }
            state = START;
            break;
    }
}

void handleGamePlayerState(GameState newGameState, PlayerStatus newPlayerStatus, int newSensitivity) {
    game.setState(newGameState);
    player.setStatus(newPlayerStatus);
    game.setSensitivity(newSensitivity);

#ifdef DEBUG
    switch (game.getState()) {
        case GAME_BEGIN:
            Serial.print("Game begin");
            break;
        case RED:
            Serial.print("Red light");
            break;
        case GREEN:
            Serial.print("Green light");
            break;
        case GAME_OVER:
            Serial.print("Game over");
            break;
    }
    switch (player.getStatus()) {
        case PLAYING:
            Serial.print(" Playing");
            break;
        case NOT_PLAYING:
            Serial.print(" Not playing");
            break;
        case MOVED:
            Serial.print(" Moved");
            break;
        case CROSSED_FINISH_LINE:
            Serial.print(" Crossed finish line");
            break;
    }

    Serial.println(" Sensitivity: " + String(game.getSensitivity()));
#endif
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

