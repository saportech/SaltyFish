#ifndef IMU_H
#define IMU_H

#include <Wire.h>
#include <Arduino.h>
#include "ICM42670P.h"

class IMU {
  public:
    bool begin();
    bool isMovementDetected(int threshold);

  private:
    ICM42670 _imu = ICM42670(Wire, 0);
    void readSensorData(float* ax, float* ay, float* az, float* gx, float* gy, float* gz);
};

#endif
