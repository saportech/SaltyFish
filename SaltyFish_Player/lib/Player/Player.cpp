#include "Player.h"
#define DEBUG

const char* predefinedMacs[] = {
    "20:43:A8:E0:59:B0",//1
    "20:43:A8:E0:5F:48",//2
    "20:43:A8:E0:4F:08",//3
    "20:43:A8:E1:65:68",//4
    "20:43:A8:E1:30:C8",//5
    "20:43:A8:E0:5E:C8",//6
    "20:43:A8:E1:F5:BC",//7
    "20:43:A8:E0:7C:78",//8
    "20:43:A8:E1:77:F8",//9
    "20:43:A8:E1:AA:B0" //10
};

const int predefinedIds[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10
};

const int numPlayers = sizeof(predefinedMacs) / sizeof(predefinedMacs[0]);

Player::Player() : _status(IDLE) {

}

void Player::begin() {
    mpu.begin();
    assignIdFromMac();
}

void Player::setStatus(PlayerStatus status) {
    _status = status;
}

PlayerStatus Player::getStatus() {
    return _status;
}

int Player::getId() {
    return _id;
}

bool Player::movedDuringRedLight(int threshold) {
    // Ensure threshold is within an acceptable range
    if (threshold < 1 || threshold > 9) {
        threshold = 8;
    }

    return mpu.isMovementDetected(threshold);
}

void Player::assignIdFromMac() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    #ifdef DEBUG
    Serial.print("Retrieved MAC address: ");
    Serial.println(macStr);
    #endif
    for (int i = 0; i < numPlayers; ++i) {
        #ifdef DEBUG
        Serial.println(predefinedMacs[i]);
        #endif
        if (strcmp(macStr, predefinedMacs[i]) == 0) {
            _id = predefinedIds[i];
            #ifdef DEBUG
            Serial.print("Assigned ID ");
            Serial.print(_id);
            Serial.print(" for MAC address ");
            Serial.println(macStr);
            #endif
            return;
        }
    }

    // Default ID if MAC address is not found in the list
    _id = 99;
    #ifdef DEBUG
    Serial.print("Assigned default ID ");
    Serial.print(_id);
    Serial.print(" for MAC address ");
    Serial.println(macStr);
    #endif
}
