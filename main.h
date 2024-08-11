#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "stm32h7xx.h"

// Function prototypes
void SystemClock_Config(void);
void Error_Handler(void);

// Stabilization functions
void initializeStabilization(void);
void updateStabilization(void);

// GPS functions
void initializeGPS(void);
void updateGPSData(void);

// Sensor data structures
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
