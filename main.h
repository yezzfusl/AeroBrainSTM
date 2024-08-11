#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "stm32h7xx.h"

void SystemClock_Config(void);
void Error_Handler(void);

void initializeStabilization(void);
void updateStabilization(void);
void setStabilizationTarget(float roll, float pitch, float yaw);

void initializeGPS(void);
void updateGPSData(void);

void initializePathPlanning(void);
void addWaypoint(double latitude, double longitude, float altitude);
void updatePathPlanning(void);

void initializeLidar(void);
void updateLidarData(void);
float getObstacleDistance(float bearing);

typedef struct {
    float roll;
    float pitch;
    float yaw;
} Orientation;

typedef struct {
    double latitude;
    double longitude;
    float altitude;
} GPSData;

extern Orientation currentOrientation;
extern GPSData currentGPSData;

#endif // MAIN_H
