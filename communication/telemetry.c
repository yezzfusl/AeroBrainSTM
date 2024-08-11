#include "../main.h"
#include <string.h>
#include <stdio.h>

#define TELEMETRY_BUFFER_SIZE 256

UART_HandleTypeDef huart2;
static uint8_t telemetryBuffer[TELEMETRY_BUFFER_SIZE];

void initializeTelemetry(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

void sendTelemetryData(void) {
    snprintf((char*)telemetryBuffer, TELEMETRY_BUFFER_SIZE,
             "GPS: %.6f, %.6f, %.2f\r\n"
             "Orientation: %.2f, %.2f, %.2f\r\n"
             "Waypoint: %d/%d\r\n",
             currentGPSData.latitude, currentGPSData.longitude, currentGPSData.altitude,
             currentOrientation.roll, currentOrientation.pitch, currentOrientation.yaw,
             getCurrentWaypointIndex(), getTotalWaypoints());

    HAL_UART_Transmit(&huart2, telemetryBuffer, strlen((char*)telemetryBuffer), HAL_MAX_DELAY);
}

void receiveTelemetryCommands(void) {
    if (HAL_UART_Receive(&huart2, telemetryBuffer, TELEMETRY_BUFFER_SIZE, 10) == HAL_OK) {
        // Process received commands
        if (strncmp((char*)telemetryBuffer, "ADD_WAYPOINT", 12) == 0) {
            double lat, lon;
            float alt;
            if (sscanf((char*)telemetryBuffer, "ADD_WAYPOINT %lf %lf %f", &lat, &lon, &alt) == 3) {
                addWaypoint(lat, lon, alt);
            }
        } else if (strcmp((char*)telemetryBuffer, "START_MISSION") == 0) {
            startMission();
        } else if (strcmp((char*)telemetryBuffer, "ABORT_MISSION") == 0) {
            abortMission();
        }
    }
}
