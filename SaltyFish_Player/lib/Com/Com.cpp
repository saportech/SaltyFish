#include "Com.h"

//#define DEBUG 1

bool Com::messageReceived = false;
bool Com::isBrainNearby = false;
Com::Msg Com::incomingMessage = {};

Com::Com() {}

void Com::begin(int id) {
    playerId = id;
    currentEstablishState = ComState::WaitingForEstablishMessage;
    resetMsg();

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    #ifdef DEBUG
    Serial.print("Device MAC Address: ");
    Serial.println(WiFi.macAddress());
    #endif

    esp_now_register_recv_cb(onDataReceive);
    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, brainMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(brainMac)) {
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add brain as a peer");
        } else {
            Serial.println("Brain added as peer");
        }
    }

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);

}

void Com::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA) {
        const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buf;

        // Access the 802.11 frame
        const uint8_t* srcMac = ppkt->payload + 10; // Source MAC address is at offset 10

        // MAC address to match
        const uint8_t targetMac[6] = {0x34, 0x5F, 0x45, 0x33, 0x5C, 0xD0};
        // Compare the source MAC with the target MAC
        if (memcmp(srcMac, targetMac, 6) == 0) {
            int rssi = ppkt->rx_ctrl.rssi; // Get the RSSI

            // Update isBrainNearby based on RSSI range
            if (rssi > -20 && rssi < 0) {
                isBrainNearby = true;
            } else {
                isBrainNearby = false;
            }

            // Debug print
            // Serial.print("RSSI from target MAC: ");
            // Serial.println(rssi);
        }
    }
}

bool Com::checkBrainNearby() const {
    return isBrainNearby;
}

void Com::receiveData() {
    if (messageReceived) {
        parseMessage(reinterpret_cast<const uint8_t*>(&incomingMessage), sizeof(incomingMessage));
        messageReceived = false;
    }
}

Com::Msg Com::getMsg() {
    return message;
}

void Com::sendMessage(int id_sender, int id_receiver, int sensitivity, GameState game_state, PlayerStatus player_status) {
    Msg msg = {id_sender, id_receiver, sensitivity, game_state, player_status};
    uint8_t data[sizeof(Msg)];
    memcpy(data, &msg, sizeof(Msg));

    esp_err_t result = esp_now_send(brainMac, data, sizeof(data));
    if (result == ESP_OK) {
        #ifdef DEBUG
        Serial.println("Message sent successfully:");
        printMessageDetails(msg);
        #endif
    } else {
        Serial.println("Error sending the message");
    }
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
        //Serial.println("Message received:");
        //printMessageDetails(incomingMessage);
        #endif
    } else {
        Serial.println("Invalid message length received");
    }
}

void Com::parseMessage(const uint8_t *data, int len) {
    if (len != sizeof(Msg)) {
        Serial.println("Invalid message length");
        return;
    }

    Msg tempMessage;
    memcpy(&tempMessage, data, len);        

    // Only update the message if sender is BrainId and receiver matches playerId
    if (tempMessage.id_sender == BrainId && tempMessage.id_receiver == playerId) {
        memcpy(&message, &tempMessage, sizeof(Msg)); // Update the actual message
        #ifdef DEBUG
        Serial.println("Message updated:");
        printMessageDetails(message);
        #endif
    } else {
        #ifdef DEBUG
        Serial.println("Message ignored due to mismatched sender or receiver.");
        #endif
    }
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

void Com::printMessageDetails(const Msg& message) {
    Serial.print(" Send: " + String(message.id_sender));
    Serial.print(" Recv: " + String(message.id_receiver));
    Serial.print(" Sens: " + String(message.sensitivity));
    Serial.print(" Game: " + String(gameStateToString(message.game_state)));
    Serial.println(" Player: " + String(playerStatusToString(message.player_status)));
}

void Com::onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
    #ifdef DEBUG
    Serial.print("Message sent status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    #endif
}

bool Com::establishedCommunication(Msg message, int playerId) {
    static unsigned long lastMillis = millis();

    switch (currentEstablishState) {
        case ComState::WaitingForEstablishMessage:
            if (message.id_sender == BrainId && message.id_receiver == playerId && message.game_state == PRE_GAME) {
                currentEstablishState = ComState::SendingEstablishMessage;
                sendMessage(playerId, BrainId, 7, PRE_GAME, IDLE);
                lastMillis = millis();
            }
            break;
        case ComState::SendingEstablishMessage:
            if (millis() - lastMillis > 3000) {
                sendMessage(playerId, BrainId, 7, PRE_GAME, IDLE);
                lastMillis = millis();
            }
            if (message.id_sender == BrainId && message.id_receiver == playerId && message.game_state == PRE_GAME) {
                currentEstablishState = ComState::Completed;
            }
            break;
        case ComState::Completed:
            return true;
    }
    return false;
}

void Com::resetEstablishedCommunication() {
    currentEstablishState = ComState::WaitingForEstablishMessage;
}