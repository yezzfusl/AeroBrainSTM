#include "../main.h"

#define LIDAR_ANGLES 360
#define MAX_LIDAR_DISTANCE 100.0f // meters

static float lidarDistances[LIDAR_ANGLES];

void initializeLidar(void) {
    for (int i = 0; i < LIDAR_ANGLES; i++) {
        lidarDistances[i] = MAX_LIDAR_DISTANCE;
    }
}

void updateLidarData(void) {
    for (int i = 0; i < LIDAR_ANGLES; i++) {
        if (i > 45 && i < 135) {
            lidarDistances[i] = 2.0f + (float)rand() / RAND_MAX * 3.0f;
        } else {
            lidarDistances[i] = MAX_LIDAR_DISTANCE;
        }
    }
}

float getObstacleDistance(float bearing) {
    int index = (int)bearing % LIDAR_ANGLES;
    return lidarDistances[index];
}
