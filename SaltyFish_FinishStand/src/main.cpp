#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>


#define SEND_INTERVAL 50
const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void sendStuff();

void setup() {
    Serial.begin(115200);

    // Init WiFi in station mode
    WiFi.mode(WIFI_STA);
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Add broadcast peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(broadcastAddress)) {
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add broadcast peer");
            return;
        }
    }
}

void loop() {

    sendStuff();

}

void sendStuff() {
    static unsigned long lastSendTime = 0;

    if (millis() - lastSendTime >= SEND_INTERVAL) {
        lastSendTime = millis();

        const char *hello = "Hello";
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)hello, strlen(hello));

        if (result == ESP_OK) {
            //Serial.println("Sent: Hello");
        } else {
            Serial.println("Failed to send");
        }
    }
}
