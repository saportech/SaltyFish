#include "Com.h"

#define DEBUG 1

bool Com::messageReceived = false;
Com::Msg Com::incomingMessage = {};

const char* Com::predefinedMacs[10] = {  // Definition in one source file
    "20:43:a8:e0:59:b0",//1
    "20:43:a8:e0:5f:48",//2
    "20:43:a8:e0:4f:08",//3
    "20:43:a8:e1:65:68",//4
    "20:43:a8:e1:30:c8",//5
    "20:43:a8:e0:5e:c8",//6
    "20:43:a8:e1:f5:bc",//7
    "20:43:a8:e0:7c:78",//8
    "20:43:a8:e1:77:f8",//9
    "20:43:a8:e1:aa:b0" //10
};

Com::Com() {}

void Com::begin() {
    resetMsg();
    WiFi.mode(WIFI_STA);

    #ifdef DEBUG
    Serial.print("Device MAC Address: ");
    Serial.println(WiFi.macAddress());
    #endif

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(onDataReceive);
    esp_now_register_send_cb(onDataSent);

    establishedCommunicationIsValid = false;

    addPlayersAsPeers();  // Add players as peers during initialization
}

void Com::addPlayersAsPeers() {
    // Add broadcast address as a peer
    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t broadcastPeerInfo = {};
    memcpy(broadcastPeerInfo.peer_addr, broadcastAddress, 6);
    broadcastPeerInfo.channel = 0;  // Use the same channel as the network
    broadcastPeerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(broadcastAddress)) {
        if (esp_now_add_peer(&broadcastPeerInfo) == ESP_OK) {
            #ifdef DEBUG
            Serial.println("Broadcast address added as peer");
            #endif
        } else {
            Serial.println("Failed to add broadcast address as peer");
        }
    }

    // Add each player as a peer
    for (int i = 0; i < MAX_PLAYERS && predefinedMacs[i] != nullptr; ++i) {
        uint8_t peerMac[6];
        sscanf(predefinedMacs[i], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &peerMac[0], &peerMac[1], &peerMac[2],
               &peerMac[3], &peerMac[4], &peerMac[5]);

        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, peerMac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (!esp_now_is_peer_exist(peerMac)) {
            if (esp_now_add_peer(&peerInfo) == ESP_OK) {
                #ifdef DEBUG
                Serial.printf("Player with MAC %s added as peer\n", predefinedMacs[i]);
                #endif
            } else {
                Serial.printf("Failed to add player with MAC %s\n", predefinedMacs[i]);
            }
        }
    }
}

void Com::sendMessage(int id_sender, int id_receiver, int sensitivity, GameState game_state, PlayerStatus player_status) {
    Msg msg = {id_sender, id_receiver, sensitivity, game_state, player_status};
    uint8_t data[sizeof(Msg)];
    memcpy(data, &msg, sizeof(Msg));

    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Broadcast address

    esp_err_t result = esp_now_send(broadcastAddress, data, sizeof(data));  // Send to broadcast address
    if (result == ESP_OK) {
        #ifdef DEBUG
        //Serial.println("Broadcast message sent successfully:");
        //printMessageDetails(msg);
        #endif
    } else {
        Serial.println("Error broadcasting the message");
    }
}

void Com::receiveData() {
    if (messageReceived) {
        parseMessage(reinterpret_cast<const uint8_t*>(&incomingMessage), sizeof(incomingMessage));
        messageReceived = false;
    }
}

void Com::parseMessage(const uint8_t *data, int len) {
    if (len != sizeof(Msg)) {
        Serial.println("Invalid message length");
        return;
    }

    memcpy(&message, data, len);

    #ifdef DEBUG
    printMessageDetails(message);
    #endif
}

void Com::printMessageDetails(const Msg& message) {
    Serial.print(" Send: " + String(message.id_sender));
    Serial.print(" Recv: " + String(message.id_receiver));
    Serial.print(" Sens: " + String(message.sensitivity));
    Serial.print(" Game: " + String(gameStateToString(message.game_state)));
    Serial.println(" Player: " + String(playerStatusToString(message.player_status)));
}

const char* Com::gameStateToString(GameState state) {
    switch (state) {
        case PRE_GAME: return "PRE_GAME";
        case GAME_BEGIN: return "GAME_BEGIN";
        case RED: return "RED";
        case GREEN: return "GREEN";
        case GAME_OVER: return "GAME_OVER";
        default: return "UNKNOWN";
    }
}

const char* Com::playerStatusToString(PlayerStatus status) {
    switch (status) {
        case IDLE: return "IDLE";
        case ESTABLISHED_COMMUNICATION: return "ESTABLISHED_COMMUNICATION";
        case PLAYING: return "PLAYING";
        case NOT_PLAYING: return "NOT_PLAYING";
        case MOVED: return "MOVED";
        case CROSSED_FINISH_LINE: return "CROSSED_FINISH_LINE";
        default: return "UNKNOWN";
    }
}

Com::Msg Com::getMsg() {
    return message;
}

void Com::resetMsg() {
    message.id_sender = 0;
    message.id_receiver = 0;
    message.sensitivity = 0;
    message.game_state = PRE_GAME;
    message.player_status = IDLE;
}

void Com::onDataReceive(const uint8_t *mac, const uint8_t *data, int len) {
    if (len == sizeof(Msg)) {
        memcpy(&incomingMessage, data, len);
        messageReceived = true;

        #ifdef DEBUG
        Serial.println("Message received:");
        //printMessageDetails(incomingMessage);
        #endif
    } else {
        Serial.println("Invalid message length received");
    }
}

void Com::onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
    #ifdef DEBUG
    //Serial.print("Message sent status: ");
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    #endif
}

int Com::establishedCommunication(Msg message, const std::vector<int>& playerIDs) {
    enum ESTABLISH_STATE_TYPE { 
        ESTABLISHING_COMMUNICATION,
        COMPLETED
    };
    
    static int establishCurrentState = ESTABLISHING_COMMUNICATION;
    static bool allAcknowledged = false;
    
    switch (establishCurrentState) {
        case ESTABLISHING_COMMUNICATION:
            for (size_t i = 0; i < playerIDs.size(); ++i) {
                if (message.id_sender == playerIDs[i] && message.game_state == PRE_GAME && message.player_status == IDLE) {
                    sendMessage(brainId, playerIDs[i], 7, PRE_GAME, IDLE);

                    #ifdef DEBUG
                    Serial.println("Player " + String(playerIDs[i]) + " acknowledged");
                    #endif

                    playerAcknowledged[i] = true;
                    return i + 1;
                }
            }
            allAcknowledged = true;
            for (size_t i = 0; i < playerIDs.size(); ++i) {
                if (!playerAcknowledged[i]) {
                    allAcknowledged = false;
                    break;
                }
            }
            if (allAcknowledged) {
                establishCurrentState = COMPLETED;
                for (size_t i = 0; i < playerIDs.size(); ++i) {
                    playerAcknowledged[i] = false;
                }
                establishedCommunicationIsValid = true;
            }
            break;
        case COMPLETED:
            if (establishedCommunicationIsValid) {
                return brainId;
            } else {
                establishCurrentState = ESTABLISHING_COMMUNICATION;
            }
            break;
    }

    return 0;
}

void Com::resetEstablishedCommunication() {
    establishedCommunicationIsValid = false;
}
