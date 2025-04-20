#include "IMU.h"

bool IMU::begin() {
    int ret = _imu.begin();
    if (ret != 0) {
        Serial.print("ICM42670 initialization failed: ");
        Serial.println(ret);
        ESP.restart();
    } else {
        Serial.println("ICM42670 initialization successful.");
    }
    _imu.startAccel(100, 16);
    _imu.startGyro(100, 2000);
    delay(100);
    return true;
}

void IMU::readSensorData(float* ax, float* ay, float* az, float* gx, float* gy, float* gz) {
    inv_imu_sensor_event_t imu_event;
    _imu.getDataFromRegisters(imu_event);

    *ax = imu_event.accel[0];
    *ay = imu_event.accel[1];
    *az = imu_event.accel[2];
    *gx = imu_event.gyro[0];
    *gy = imu_event.gyro[1];
    *gz = imu_event.gyro[2];
}

bool IMU::isMovementDetected(int userThreshold) {
    //4000 almost every movement, 10000 medium, 15000 almost impossible to reach
    int threshold = map(userThreshold, 3, 1, 4000, 15000);
    //int threshold = 8000;

    static unsigned long lastSampleTime = 0;
    unsigned long currentTime = millis();
    #define SAMPLE_TIME 50

    if (currentTime - lastSampleTime >= SAMPLE_TIME) {
        float ax, ay, az, gx, gy, gz;
        readSensorData(&ax, &ay, &az, &gx, &gy, &gz);

        // Calculate the absolute sum of sensor values
        int absoluteSum = abs(ax) + abs(ay) + abs(az) + abs(gx) + abs(gy) + abs(gz);

        lastSampleTime = currentTime;

        return (absoluteSum > threshold);
    }

    return false;
}


// bool IMU::isMovementDetected(int threshold) {
//     threshold = 1500;//100 was good -1000
//     #define SAMPLE_TIME 50

//     static unsigned long lastSampleTime = 0;
//     static bool initialized = false;

//     static float prevAx, prevAy, prevAz;
//     static float prevGx, prevGy, prevGz;

//     unsigned long currentTime = millis();


//     if (currentTime - lastSampleTime >= SAMPLE_TIME) {
//         lastSampleTime = currentTime;

//         float ax, ay, az, gx, gy, gz;
//         readSensorData(&ax, &ay, &az, &gx, &gy, &gz);

//         if (!initialized) {
//             prevAx = ax; prevAy = ay; prevAz = az;
//             prevGx = gx; prevGy = gy; prevGz = gz;
//             initialized = true;
//             return false;
//         }

//         Serial.print("Sensor values: ");
//         Serial.print("ax: "); Serial.print(ax); Serial.print(" ");
//         Serial.print("ay: "); Serial.print(ay); Serial.print(" ");
//         Serial.print("az: "); Serial.print(az); Serial.print(" ");
//         Serial.print("gx: "); Serial.print(gx); Serial.print(" ");
//         Serial.print("gy: "); Serial.print(gy); Serial.print(" ");
//         Serial.print("gz: "); Serial.println(gz);

//         if (abs(ax - prevAx) > threshold) {
//             Serial.println("Movement detected: ax");
//             prevAx = ax; prevAy = ay; prevAz = az;
//             prevGx = gx; prevGy = gy; prevGz = gz;
//             return true;
//         }
//         if (abs(ay - prevAy) > threshold) {
//             Serial.println("Movement detected: ay");
//             prevAx = ax; prevAy = ay; prevAz = az;
//             prevGx = gx; prevGy = gy; prevGz = gz;
//             return true;
//         }
//         if (abs(az - prevAz) > threshold) {
//             Serial.println("Movement detected: az");
//             prevAx = ax; prevAy = ay; prevAz = az;
//             prevGx = gx; prevGy = gy; prevGz = gz;
//             return true;
//         }
//         if (abs(gx - prevGx) > threshold) {
//             Serial.println("Movement detected: gx");
//             prevAx = ax; prevAy = ay; prevAz = az;
//             prevGx = gx; prevGy = gy; prevGz = gz;
//             return true;
//         }
//         if (abs(gy - prevGy) > threshold) {
//             Serial.println("Movement detected: gy");
//             prevAx = ax; prevAy = ay; prevAz = az;
//             prevGx = gx; prevGy = gy; prevGz = gz;
//             return true;
//         }
//         if (abs(gz - prevGz) > threshold) {
//             Serial.println("Movement detected: gz");
//             prevAx = ax; prevAy = ay; prevAz = az;
//             prevGx = gx; prevGy = gy; prevGz = gz;
//             return true;
//         }

//         // No movement detected, still update for next time
//         prevAx = ax; prevAy = ay; prevAz = az;
//         prevGx = gx; prevGy = gy; prevGz = gz;
//     }

//     return false;
// }