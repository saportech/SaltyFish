#include "Player.h"

Player::Player() : _id(-1), _status(IDLE) {
    // Initialize default values
}

void Player::begin(int id) {
    _id = id;
    _status = IDLE;
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