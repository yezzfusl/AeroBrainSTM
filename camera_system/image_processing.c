#include "../main.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdlib.h>

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480
#define IMAGE_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT)

static uint8_t imageBuffer[IMAGE_SIZE];
static uint8_t processedImage[IMAGE_SIZE];

// DCMI handle
DCMI_HandleTypeDef hdcmi;

// I2C handle for camera configuration
I2C_HandleTypeDef hi2c1;

// DMA handle for DCMI
DMA_HandleTypeDef hdma_dcmi;

// OV7670 configuration registers
#define OV7670_I2C_ADDR 0x42
#define OV7670_REG_COM7 0x12
#define OV7670_REG_COM3 0x0C
#define OV7670_REG_COM14 0x3E
#define OV7670_REG_SCALING_XSC 0x70
#define OV7670_REG_SCALING_YSC 0x71
#define OV7670_REG_SCALING_DCWCTR 0x72
#define OV7670_REG_SCALING_PCLK_DIV 0x73

static HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;
    return HAL_I2C_Master_Transmit(&hi2c1, OV7670_I2C_ADDR, buf, 2, HAL_MAX_DELAY);
}

void initializeImageProcessing(void)
{
    // Initialize I2C
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure OV7670
    HAL_Delay(100); // Wait for camera to initialize
    OV7670_WriteReg(OV7670_REG_COM7, 0x80); // Reset all registers
    HAL_Delay(100);
    OV7670_WriteReg(OV7670_REG_COM7, 0x00); // VGA
    OV7670_WriteReg(OV7670_REG_COM3, 0x04); // DCW enable
    OV7670_WriteReg(OV7670_REG_COM14, 0x19); // Scaling PCLK setup
    OV7670_WriteReg(OV7670_REG_SCALING_XSC, 0x3A);
    OV7670_WriteReg(OV7670_REG_SCALING_YSC, 0x35);
    OV7670_WriteReg(OV7670_REG_SCALING_DCWCTR, 0x11);
    OV7670_WriteReg(OV7670_REG_SCALING_PCLK_DIV, 0xF1);

    // Initialize DCMI
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
    if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
    {
        Error_Handler();
    }

    // Initialize DMA for DCMI
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
    if (HAL_DMA_Init(&hdma_dcmi) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&hdcmi, DMA_Handle, hdma_dcmi);
}

void captureImage(void)
{
    HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)imageBuffer, IMAGE_SIZE / 4);
    while (HAL_DCMI_GetState(&hdcmi) == HAL_DCMI_STATE_BUSY) {}
    HAL_DCMI_Stop(&hdcmi);
}

void processImage(void)
{
    // Simple edge detection using Sobel operator
    int sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    memset(processedImage, 0, IMAGE_SIZE);

    for (int y = 1; y < IMAGE_HEIGHT - 1; y++)
    {
        for (int x = 1; x < IMAGE_WIDTH - 1; x++)
        {
            int pixelX = 0, pixelY = 0;
            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    int pixel = imageBuffer[(y + i) * IMAGE_WIDTH + (x + j)];
                    pixelX += pixel * sobelX[i + 1][j + 1];
                    pixelY += pixel * sobelY[i + 1][j + 1];
                }
            }
            int magnitude = (int)sqrt(pixelX * pixelX + pixelY * pixelY);
            processedImage[y * IMAGE_WIDTH + x] = (magnitude > 128) ? 255 : 0;
        }
    }
}

void analyzeImage(void)
{
    int whitePixels = 0;
    for (int i = 0; i < IMAGE_SIZE; i++)
    {
        if (processedImage[i] == 255)
        {
            whitePixels++;
        }
    }
    float edgePercentage = (float)whitePixels / IMAGE_SIZE * 100;

    // Simple analysis: if more than 10% of the image is edges, consider it a complex scene
    if (edgePercentage > 10)
    {
        setComplexScene(1);
    }
    else
    {
        setComplexScene(0);
    }
}

void runImageProcessing(void)
{
    captureImage();
    processImage();
    analyzeImage();
}
