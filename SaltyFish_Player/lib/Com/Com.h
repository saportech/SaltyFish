#ifndef COM_H
#define COM_H

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "Game.h"
#include "Player.h"

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
    void begin(int id);
    void reinit(int id);
    void deinitEspNow();
    void receiveData();
    Msg getMsg();
    void sendMessage(int id_sender, int id_receiver, int sensitivity, GameState game_state, PlayerStatus player_status);
    bool establishedCommunication(Msg message, int playerId);
    void resetMsg();
    void resetEstablishedCommunication();
    static void onDataReceive(const uint8_t *mac, const uint8_t *data, int len);
    static void onDataSent(const uint8_t *mac, esp_now_send_status_t status);

    const char* gameStateToString(GameState state);
    const char* playerStatusToString(PlayerStatus status);

    bool checkBrainNearby() const;

private:
    void parseMessage(const uint8_t *data, int len);
    void printMessageDetails(const Msg& message);

    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);

    Msg message;
    int playerId;
    static bool isBrainNearby;

    enum class ComState {
        WaitingForEstablishMessage,
        SendingEstablishMessage,
        Completed
    };
    ComState currentEstablishState;
    static bool messageReceived;
    static Msg incomingMessage;

    const uint8_t brainMac[6] = {0x34, 0x5F, 0x45, 0x33, 0x5C, 0xD0};
    int BrainId = 20;
};

#endif
