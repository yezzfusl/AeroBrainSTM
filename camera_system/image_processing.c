#include "../main.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <arm_math.h>
#include <arm_nnfunctions.h>

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480
#define IMAGE_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT)
#define CONV_KERNEL_SIZE 3
#define POOL_SIZE 2
#define FEATURE_MAPS 16
#define CONV_OUTPUT_SIZE ((IMAGE_WIDTH - CONV_KERNEL_SIZE + 1) * (IMAGE_HEIGHT - CONV_KERNEL_SIZE + 1))
#define POOL_OUTPUT_SIZE (CONV_OUTPUT_SIZE / (POOL_SIZE * POOL_SIZE))

static uint8_t imageBuffer[IMAGE_SIZE];
static float32_t processedImage[IMAGE_SIZE];
static float32_t convOutput[FEATURE_MAPS][CONV_OUTPUT_SIZE];
static float32_t poolOutput[FEATURE_MAPS][POOL_OUTPUT_SIZE];

DCMI_HandleTypeDef hdcmi;
I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_dcmi;

#define OV7670_I2C_ADDR 0x42
#define OV7670_REG_COM7 0x12
#define OV7670_REG_COM3 0x0C
#define OV7670_REG_COM14 0x3E
#define OV7670_REG_SCALING_XSC 0x70
#define OV7670_REG_SCALING_YSC 0x71
#define OV7670_REG_SCALING_DCWCTR 0x72
#define OV7670_REG_SCALING_PCLK_DIV 0x73

static float32_t convKernels[FEATURE_MAPS][CONV_KERNEL_SIZE][CONV_KERNEL_SIZE];
static float32_t convBias[FEATURE_MAPS];
static arm_nnactivation_instance_q7 reluParams;
static q7_t aiModelWeights[10000];
static q7_t aiModelBias[100];
static arm_fully_connected_instance_q7 fcParams;

static HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    return HAL_I2C_Master_Transmit(&hi2c1, OV7670_I2C_ADDR, buf, 2, HAL_MAX_DELAY);
}

void initializeImageProcessing(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);

    HAL_Delay(100);
    OV7670_WriteReg(OV7670_REG_COM7, 0x80);
    HAL_Delay(100);
    OV7670_WriteReg(OV7670_REG_COM7, 0x00);
    OV7670_WriteReg(OV7670_REG_COM3, 0x04);
    OV7670_WriteReg(OV7670_REG_COM14, 0x19);
    OV7670_WriteReg(OV7670_REG_SCALING_XSC, 0x3A);
    OV7670_WriteReg(OV7670_REG_SCALING_YSC, 0x35);
    OV7670_WriteReg(OV7670_REG_SCALING_DCWCTR, 0x11);
    OV7670_WriteReg(OV7670_REG_SCALING_PCLK_DIV, 0xF1);

    hdcmi.Instance = DCMI;
    hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
    hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_FALLING;
    hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
    hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
    hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
    hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
    hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
    hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
    hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
    hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
    HAL_DCMI_Init(&hdcmi);

    __HAL_RCC_DMA1_CLK_ENABLE();
    hdma_dcmi.Instance = DMA1_Stream0;
    hdma_dcmi.Init.Request = DMA_REQUEST_DCMI;
    hdma_dcmi.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_dcmi.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dcmi.Init.MemInc = DMA_MINC_ENABLE;
    hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_dcmi.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_dcmi.Init.Mode = DMA_CIRCULAR;
    hdma_dcmi.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_dcmi.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_dcmi.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_dcmi.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_dcmi.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&hdma_dcmi);

    __HAL_LINKDMA(&hdcmi, DMA_Handle, hdma_dcmi);

    for (int i = 0; i < FEATURE_MAPS; i++) {
        for (int j = 0; j < CONV_KERNEL_SIZE; j++) {
            for (int k = 0; k < CONV_KERNEL_SIZE; k++) {
                convKernels[i][j][k] = ((float32_t)rand() / RAND_MAX) * 2.0f - 1.0f;
            }
        }
        convBias[i] = ((float32_t)rand() / RAND_MAX) * 0.1f;
    }

    arm_relu_init_q7(&reluParams, CONV_OUTPUT_SIZE);
    fcParams.dim_src = POOL_OUTPUT_SIZE * FEATURE_MAPS;
    fcParams.dim_dst = 1;
    fcParams.weight_col = fcParams.dim_src;
    fcParams.num_of_rows = fcParams.dim_dst;
    fcParams.pWeight = aiModelWeights;
    fcParams.bias = aiModelBias;
}

void captureImage(void) {
    HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)imageBuffer, IMAGE_SIZE / 4);
    while (HAL_DCMI_GetState(&hdcmi) == HAL_DCMI_STATE_BUSY) {}
    HAL_DCMI_Stop(&hdcmi);
}

void processImage(void) {
    arm_q7_to_float((q7_t*)imageBuffer, processedImage, IMAGE_SIZE);
    arm_scale_f32(processedImage, 1.0f / 255.0f, processedImage, IMAGE_SIZE);

    for (int f = 0; f < FEATURE_MAPS; f++) {
        for (int y = 0; y < IMAGE_HEIGHT - CONV_KERNEL_SIZE + 1; y++) {
            for (int x = 0; x < IMAGE_WIDTH - CONV_KERNEL_SIZE + 1; x++) {
                float32_t sum = convBias[f];
                for (int ky = 0; ky < CONV_KERNEL_SIZE; ky++) {
                    for (int kx = 0; kx < CONV_KERNEL_SIZE; kx++) {
                        sum += processedImage[(y + ky) * IMAGE_WIDTH + (x + kx)] * convKernels[f][ky][kx];
                    }
                }
                convOutput[f][y * (IMAGE_WIDTH - CONV_KERNEL_SIZE + 1) + x] = sum;
            }
        }
    }

    for (int f = 0; f < FEATURE_MAPS; f++) {
        arm_relu_f32(convOutput[f], CONV_OUTPUT_SIZE);
    }

    for (int f = 0; f < FEATURE_MAPS; f++) {
        for (int y = 0; y < (IMAGE_HEIGHT - CONV_KERNEL_SIZE + 1) / POOL_SIZE; y++) {
            for (int x = 0; x < (IMAGE_WIDTH - CONV_KERNEL_SIZE + 1) / POOL_SIZE; x++) {
                float32_t maxVal = -INFINITY;
                for (int py = 0; py < POOL_SIZE; py++) {
                    for (int px = 0; px < POOL_SIZE; px++) {
                        int idx = (y * POOL_SIZE + py) * (IMAGE_WIDTH - CONV_KERNEL_SIZE + 1) + (x * POOL_SIZE + px);
                        maxVal = fmaxf(maxVal, convOutput[f][idx]);
                    }
                }
                poolOutput[f][y * ((IMAGE_WIDTH - CONV_KERNEL_SIZE + 1) / POOL_SIZE) + x] = maxVal;
            }
        }
    }
}

void analyzeImage(void) {
    q7_t poolOutputQ7[FEATURE_MAPS * POOL_OUTPUT_SIZE];
    q7_t fcOutput[1];
    
    arm_float_to_q7((float32_t*)poolOutput, poolOutputQ7, FEATURE_MAPS * POOL_OUTPUT_SIZE);
    arm_fully_connected_q7(poolOutputQ7, aiModelWeights, fcParams.dim_src, fcParams.dim_dst, 0, 0, aiModelBias, fcOutput, NULL);
    
    float complexity = (float)fcOutput[0] / 127.0f;
    setComplexScene(complexity > 0.5f);
}

void runImageProcessing(void) {
    captureImage();
    processImage();
    analyzeImage();
}
