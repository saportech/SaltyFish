#ifndef Player_h
#define Player_h

#include <Arduino.h>

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
    void begin(int id);  // Modify to accept an ID parameter
    void setStatus(PlayerStatus status);
    PlayerStatus getStatus();
    int getId();

private:
    int _id;
    PlayerStatus _status;
};

#endif // PLAYER_H
