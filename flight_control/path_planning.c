#include "../main.h"
#include <math.h>
#include <stdlib.h>

#define MAX_WAYPOINTS 100
#define OBSTACLE_THRESHOLD 1.0f // meters
#define COMPLEX_SCENE_SPEED_REDUCTION 0.5f

typedef struct {
    double latitude;
    double longitude;
    float altitude;
} Waypoint;

static Waypoint waypoints[MAX_WAYPOINTS];
static int waypointCount = 0;
static int currentWaypoint = 0;
static int missionStarted = 0;
static int isComplexScene = 0;

void initializePathPlanning(void) {
    waypointCount = 0;
    currentWaypoint = 0;
    missionStarted = 0;
    isComplexScene = 0;
}

void addWaypoint(double latitude, double longitude, float altitude) {
    if (waypointCount < MAX_WAYPOINTS) {
        waypoints[waypointCount].latitude = latitude;
        waypoints[waypointCount].longitude = longitude;
        waypoints[waypointCount].altitude = altitude;
        waypointCount++;
    }
}

static double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000; // Earth radius in meters
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

static double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double y = sin(dLon) * cos(lat2 * M_PI / 180.0);
    double x = cos(lat1 * M_PI / 180.0) * sin(lat2 * M_PI / 180.0) -
               sin(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) * cos(dLon);
    return atan2(y, x) * 180.0 / M_PI;
}

void updatePathPlanning(void) {
    if (!missionStarted || currentWaypoint >= waypointCount) {
        return;
    }

    double distance = calculateDistance(currentGPSData.latitude, currentGPSData.longitude,
                                        waypoints[currentWaypoint].latitude, waypoints[currentWaypoint].longitude);
    
    if (distance < 5.0) {
        currentWaypoint++;
        return;
    }

    double bearing = calculateBearing(currentGPSData.latitude, currentGPSData.longitude,
                                      waypoints[currentWaypoint].latitude, waypoints[currentWaypoint].longitude);

    float obstacleDistance = getObstacleDistance(bearing);
    
    if (obstacleDistance < OBSTACLE_THRESHOLD) {
        bearing += 45.0;
        if (bearing > 360.0) bearing -= 360.0;
    }

    float desiredYaw = (float)bearing;
    float desiredPitch = -5.0f;
    float desiredRoll = 0.0f;

    if (isComplexScene) {
        desiredPitch *= COMPLEX_SCENE_SPEED_REDUCTION;
    }

    setStabilizationTarget(desiredRoll, desiredPitch, desiredYaw);
}

int getCurrentWaypointIndex(void) {
    return currentWaypoint;
}

int getTotalWaypoints(void) {
    return waypointCount;
}

void startMission(void) {
    missionStarted = 1;
}

void abortMission(void) {
    missionStarted = 0;
    currentWaypoint = 0;
}

void setComplexScene(int isComplex) {
    isComplexScene = isComplex;
}
