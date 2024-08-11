#include "../main.h"
#include <math.h>

#define KP 1.0f
#define KI 0.1f
#define KD 0.05f

static float integral[3] = {0};
static float lastError[3] = {0};
static float targetOrientation[3] = {0};

void initializeStabilization(void) {
}

void setStabilizationTarget(float roll, float pitch, float yaw) {
    targetOrientation[0] = roll;
    targetOrientation[1] = pitch;
    targetOrientation[2] = yaw;
}

void updateStabilization(void) {
    float currentOrientation[3] = {
        currentOrientation.roll,
        currentOrientation.pitch,
        currentOrientation.yaw
    };
    
    float output[3];
    for (int i = 0; i < 3; i++) {
        float error = targetOrientation[i] - currentOrientation[i];
        integral[i] += error;
        float derivative = error - lastError[i];
        
        output[i] = KP * error + KI * integral[i] + KD * derivative;
        
        lastError[i] = error;
    }
}
