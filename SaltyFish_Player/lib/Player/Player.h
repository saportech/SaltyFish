#ifndef Player_h
#define Player_h

#include <Arduino.h>
#include <WiFi.h>
#include "IMU.h"

enum PlayerStatus {
    IDLE,
    ESTABLISHED_COMMUNICATION,
    PLAYING,
    NOT_PLAYING,
    MOVED,
    CROSSED_FINISH_LINE
};



class Player {
public:
    Player();
    void begin();
    void setStatus(PlayerStatus status);
    PlayerStatus getStatus();
    bool movedDuringRedLight(int threshold);
    int getId();
private:
    int _id;
    PlayerStatus _status;
    IMU mpu;

    void assignIdFromMac();
};

#endif
