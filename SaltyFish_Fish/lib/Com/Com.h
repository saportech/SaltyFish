#ifndef Com_h
#define Com_h

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <vector>
#include "Game.h"
#include "Player.h"

const int predefinedIds[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10
};

enum class MessageType : int {
    ESTABLISH,
    GAME,
    UNKNOWN
};

class Com {
public:
    struct Msg {
        int id_sender;
        int id_receiver;
        int sensitivity;
        GameState game_state;
        PlayerStatus player_status;
    };

    Com();
    void begin();
    void receiveData();
    Msg getMsg();
    void sendMessage(int id_sender, int id_receiver, int sensitivity, GameState game_state, PlayerStatus player_status);
    int establishedCommunication(Msg message, const std::vector<int>& playerIDs);
    void resetMsg();
    void resetEstablishedCommunication();
    int brainId = 20;
private:
    Msg message;
    static void onDataReceive(const uint8_t *mac, const uint8_t *data, int len);
    static void onDataSent(const uint8_t *mac, esp_now_send_status_t status);

    void parseMessage(const uint8_t *data, int len);
    void printMessageDetails(const Msg& message);

    static const char* gameStateToString(GameState state);
    static const char* playerStatusToString(PlayerStatus status);

    void addPlayersAsPeers();

    static bool messageReceived;
    static Msg incomingMessage;
    bool establishedCommunicationIsValid;

    static const size_t MAX_PLAYERS = 10;
    bool playerAcknowledged[MAX_PLAYERS];

    static const char* predefinedMacs[10];

    

};

#endif
