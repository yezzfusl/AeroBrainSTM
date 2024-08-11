# AeroBrainSTM
Autonomous STM32H743ZI Drone System

## Hardware Requirements

- STM32H743ZI microcontroller board
- GPS module (e.g., NEO-6M)
- LIDAR sensor (e.g., TFMini Plus)
- OV7670 camera module
- Telemetry module (e.g., XBee)
- IMU (Inertial Measurement Unit)
- Drone frame and motors

## Software Requirements

- STM32CubeIDE (version 1.8.0 or later)
- ARM GCC Toolchain
- OpenOCD for flashing
## Key Features

1. **Flight Control**: Implements PID-based stabilization for roll, pitch, and yaw.
2. **GPS Navigation**: Utilizes GPS data for waypoint navigation.
3. **Obstacle Avoidance**: Uses LIDAR data to detect and avoid obstacles.
4. **Path Planning**: Generates and follows optimal paths between waypoints.
5. **Telemetry**: Enables real-time data transmission and remote command reception.
6. **Computer Vision**: Incorporates image processing for scene analysis and decision-making.

## Build Instructions

1. Clone the repository:

`git clone https://github.com/yezzfusl/AeroBrainSTM.git`

`cd AeroBrainSTM`

2. Build the project:

`make`

3. Flash the binary to the STM32H743ZI:

`openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program build/autonomous_drone.elf verify reset exit"`

## Usage

1. Power on the drone system.
2. Use the telemetry interface to:
- Add waypoints
- Start the mission
- Monitor drone status
- Abort the mission if necessary

## Technical Details

- **Microcontroller**: STM32H743ZI (Arm Cortex-M7, 400 MHz, 2 MB Flash, 1 MB RAM)
- **GPS**: UART communication, NMEA sentence parsing
- **LIDAR**: I2C or UART communication, distance measurement up to 12m
- **Camera**: OV7670 module, DCMI interface, 640x480 resolution
- **Telemetry**: UART communication, custom protocol for data exchange
- **Image Processing**: Basic edge detection using Sobel operator
- **Path Planning**: A* algorithm for optimal path generation
- **Obstacle Avoidance**: Reactive avoidance based on LIDAR readings

## Performance Considerations

- The main control loop runs at 100 Hz (10ms cycle time).
- Image processing is computationally intensive and may impact overall system performance.
- The system uses DMA for efficient data transfer from peripherals.

## Safety Features

- Geofencing to restrict flight area
- Failsafe routines for loss of GPS or telemetry signal
- Low battery voltage detection and auto-return

## Future Improvements

- Implement advanced computer vision algorithms for object detection and tracking
- Enhance path planning with dynamic obstacle avoidance
- Add support for multi-drone coordination

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributors

- [@YEZZFUSL]
