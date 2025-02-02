# [PWMMotorControl](https://github.com/ArminJo/PWMMotorControl)
Available as Arduino library "PWMMotorControl"

### [Version 2.0.0](https://github.com/ArminJo/PWMMotorControl/archive/master.zip) - work in progress

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Installation instructions](https://www.ardu-badge.com/badge/PWMMotorControl.svg?)](https://www.ardu-badge.com/PWMMotorControl)
[![Commits since latest](https://img.shields.io/github/commits-since/ArminJo/PWMMotorControl/latest)](https://github.com/ArminJo/PWMMotorControl/commits/master)
[![Build Status](https://github.com/ArminJo/PWMMotorControl/workflows/LibraryBuild/badge.svg)](https://github.com/ArminJo/PWMMotorControl/actions)
![Hit Counter](https://visitor-badge.laobi.icu/badge?page_id=ArminJo_PWMMotorControl)


- The PWMDcMotor.cpp controls **brushed DC motors** by PWM using standard full bridge IC's like **[L298](https://www.instructables.com/L298-DC-Motor-Driver-DemosTutorial/)**, [**SparkFun Motor Driver - Dual TB6612FNG**](https://www.sparkfun.com/products/14451), or **[Adafruit_MotorShield](https://www.adafruit.com/product/1438)** (using PCA9685 -> 2 x TB6612).
- The EncoderMotor.cpp.cpp controls a DC motor with attached encoder disc and slot-type photo interrupters to enable **driving a specified distance**.
- The CarPWMMotorControl.cpp controls **2 motors simultaneously** like it is required for most **Robot Cars**.
- To **compensate for different motor characteristics**, each motor can have a **positive** compensation value, which is **subtracted** from the requested speed PWM if you use the `setSpeedPWMCompensation()` functions. For car control, only compensation of one motor is required.

#### The motor is mainly controlled by 2 dimensions:
1. **Direction** / motor driver control. Can be FORWARD, BACKWARD, BRAKE (motor connections are shortened) or RELEASE (motor connections are high impedance).
2. **SpeedPWM** which is ignored for BRAKE or RELEASE. Some functions allow a signed speedPWM parameter, which incudes the direction as sign (positive -> FORWARD).

#### Basic commands are:
- `init(uint8_t aForwardPin, uint8_t aBackwardPin, uint8_t aPWMPin)`.
- `setSpeedPWM(uint8_t aRequestedSpeedPWM, uint8_t aRequestedDirection)` or `setSpeedPWM(int Signed_RequestedSpeedPWM)`.
- `stop()` or `setSpeedPWM(0)`.
- `startRampUp(uint8_t aRequestedDirection)`, `goDistanceMillimeter(unsigned int aRequestedDistanceMillimeter, uint8_t aRequestedDirection);`,  `startGoDistanceMillimeter(int aRequestedDistanceMillimeter)` and `updateMotor()`.
- `getSpeed()`, `getAverageSpeed()`,  `getDistanceMillimeter()` and `getBrakingDistanceMillimeter()` for encoder motors or MPU6050 IMU equipped cars.

#### To go a specified distance use:
Drving speed PWM is the PWM value to use for driving a fixed distance. The software generates a ramp up from start PWM to driving speed PWM at the start of the movement and a ramp down to stop.
- `calibrate()` to automatically set start speed for encoder or IMU supported cars.
- `startGoDistanceMillimeter(unsigned int aRequestedDistanceMillimeter, uint8_t aRequestedDirection)` or `setSpeedPWM(uint8_t aRequestedSpeedPWM, uint8_t aRequestedDirection)` - for non encoder motors a formula, using distance and the drive speed PWM, is used to convert counts into motor driving time.
- `updateMotor()` or `updateMotors()` - call this in your loop if you use the start* functions.

2 wheel car from LAVFIN with 2 LiPo batteries case, and IR receiver, with wires in original length.
![2 wheel car](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/L298Car_TopView_small.jpg)
Arduino Plotter diagram of PWM, speed[rpm] and encoder count for USB 3.8 volt supply and a Mosfet bridge. Timebase is 50 ms per plotted value. Generated by the [PrintMotorDiagram](https://github.com/ArminJo/PWMMotorControl/tree/master/examples/PrintMotorDiagram) example.
![USB powered](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/PWM_Speed_USB.png)
Diagram of PWM, speed[rpm] and encoder count for 2 LiPo (7.5 volt) supply and a Mosfet bridge.
![2 LiPo powered](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/PWM_Speed_2LiPos.png)

# Compile options / macros for this library
To customize the library to different requirements, there are some compile options / macros available.<br/>
Modify them by commenting them out or in, or change the values if applicable. Or define the macro with the -D compiler option for global compile (the latter is not possible with the Arduino IDE, so consider using [Sloeber](https://eclipse.baeyens.it).<br/>
Some options which are enabled by default can be disabled by defining a *inhibit* macro like `USE_STANDARD_LIBRARY_FOR_ADAFRUIT_MOTOR_SHIELD`.

| Macro | Default | File | Description |
|-|-|-|-|
| `USE_ENCODER_MOTOR_CONTROL` | disabled | PWMDCMotor.h | Use slot-type photo interrupter and an attached encoder disc to enable motor distance and speed sensing for closed loop control. |
| `USE_MPU6050_IMU` | disabled | CarIMUData.h | Use GY-521 MPU6050 breakout board connected by I2C for support of precise turning and speed / distance calibration. Connectors point to the rear. |
| `USE_ACCELERATOR_Y_FOR_SPEED` | undefined | CarIMUData.h | The y axis of the GY-521 MPU6050 breakout board points forward / backward, i.e. connectors are at the left / right side. |
| `USE_NEGATIVE_ACCELERATION_FOR_SPEED` | undefined | CarIMUData.h | The speed axis of the GY-521 MPU6050 breakout board points backward, i.e. connectors are at the front or right side. |
| `USE_ADAFRUIT_MOTOR_SHIELD` | disabled | PWMDcMotor.h | Use Adafruit Motor Shield v2 connected by I2C instead of simple TB6612 or L298 breakout board.<br/>This disables tone output by using motor as loudspeaker, but requires only 2 I2C/TWI pins in contrast to the 6 pins used for the full bridge.<br/>For full bridge, analogWrite the millis() timer0 is used since we use pin 5 & 6. |
| `USE_STANDARD_LIBRARY_`<br/>`ADAFRUIT_MOTOR_SHIELD` | disabled | PWMDcMotor.h | Enabling requires additionally 694 bytes program memory. |

# Default car geometry dependent values used in this library
These values are for a standard 2 WD car as can be seen on the pictures below.
| Macro | Default | File | Description |
|-|-|-|-|
| `DEFAULT_CIRCUMFERENCE_MILLIMETER` | 220 | PWMDCMotor.h | At a circumference of around 220 mm this gives 11 mm per count. |
| `ENCODER_COUNTS_PER_FULL_ROTATION` | 20 | EncoderMotor.h | This value is for 20 slot encoder discs, giving 20 on and 20 off counts per full rotation. |
| `FACTOR_DEGREE_TO_MILLIMETER_DEFAULT` | 2.2777 for 2 wheel drive cars, 5.0 for 4 WD cars | CarPWMMotorControl.h | Reflects the geometry of the standard 2 WD car sets. The 4 WD car value is estimated for slip on smooth surfaces. |

# Other default values for this library
These values are used by functions and some can be overwritten by set* functions.
| Macro | Default | File | Description |
|-|-|-|-|
| `VIN_2_LIPO` | undefined | PWMDCMotor.h | If defined sets `FULL_BRIDGE_INPUT_MILLIVOLT` to 7400. |
| `VIN_1_LIPO` | undefined | PWMDCMotor.h | If defined sets `FULL_BRIDGE_INPUT_MILLIVOLT` to 3700. |
| `FULL_BRIDGE_INPUT_`<br/>`MILLIVOLT` | 6000 or 7400 if `VIN_2_LIPO` is defined | PWMDCMotor.h | The supply voltage used for the full bridge. |
| `MOSFET_BRIDGE_USED` | undefined | PWMDCMotor.h | If defined sets `FULL_BRIDGE_LOSS_MILLIVOLT` to 0. |
| `FULL_BRIDGE_LOSS_`<br/>`MILLIVOLT` | 2000 or 0 if `FULL_BRIDGE_LOSS_MILLIVOLT` is defined | PWMDCMotor.h | The internal voltage loss of the full bridge used, typically 2 volt for bipolar bridges like the L298. |
| `FULL_BRIDGE_OUTPUT_`<br/>`MILLIVOLT` | `(FULL_BRIDGE_INPUT_MILLIVOLT - FULL_BRIDGE_LOSS_MILLIVOLT)` | PWMDCMotor.h | The effective voltage available for the motor. |
| `DEFAULT_START_`<br/>`MILLIVOLT` | 1100 | PWMDCMotor.h | The DC Voltage at which the motor start to move / dead band voltage. |
| `DEFAULT_DRIVE_`<br/>`MILLIVOLT` | 2000 | PWMDCMotor.h | The derived `DEFAULT_DRIVE_SPEED_PWM` is the speed PWM value used for fixed distance driving. |
| `DEFAULT_MILLIMETER_`<br/>`PER_SECOND` | 320 | PWMDCMotor.h | Value at DEFAULT_DRIVE_MILLIVOLT motor supply. A factor used to convert distance to motor on time in milliseconds using the formula:<br/>`computedMillisOf`<br/>`MotorStopForDistance = 150 + (10 * ((aRequestedDistanceCount * DistanceToTimeFactor) / DriveSpeedPWM))` |

### Changing include (*.h) files with Arduino IDE
First, use *Sketch > Show Sketch Folder (Ctrl+K)*.<br/>
If you have not yet saved the example as your own sketch, then you are instantly in the right library folder.<br/>
Otherwise you have to navigate to the parallel `libraries` folder and select the library you want to access.<br/>
In both cases the library source and include files are located in the libraries `src` directory.<br/>
The modification must be renewed for each new library version!

### Modifying compile options with Sloeber IDE
If you are using [Sloeber](https://eclipse.baeyens.it) as your IDE, you can easily define global symbols with *Properties > Arduino > CompileOptions*.<br/>
![Sloeber settings](https://github.com/ArminJo/ServoEasing/blob/master/pictures/SloeberDefineSymbols.png)

# Full bridges
This library was tested with the bipolar full bridge IC L298 and the MOSFET IC MTB6612.
![L298 board](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/L298Board_small.jpg)
![TB6612 board](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/TB6612Board_small.jpg)

The L298 has a loss of around 2 volt, which the reason for the attached heat sink, the MTB6612 has almost no loss.

# Internals
- PWM period is 600 �s for Adafruit Motor Shield V2 using PCA9685.
- PWM period is 1030 �s for using AnalogWrite on pin 5 + 6.

# [Examples](tree/master/examples)

## Start
One motor starts with DriveSpeedPWM / 2 for one second, then runs 1 second with DriveSpeedPWM.
After stopping the motor, it tries to run for one full rotation (resulting in a 90 degree turn for a 2WD car). Then the other motor runs the same cycle.
For the next loop, the direction is switched to backwards.

## Square
4 times drive 40 cm, then 90 degree left turn. After the square, the car is turned by 180 degree and the direction is switched to backwards. Then the square starts again.

## PrintMotorDiagram
Prints PWM, distance and speed diagram of an encoder motor. The encoder is inverted at falling PWM slope to show the quadratic kind of encoder graph.
| Diagram for free running motor controlled by an MosFet bridge supplied by 7.0 volt | Diagram for free running motor controlled by an L298 bridge supplied by 7.6 volt |
| :-: | :-: |
| ![7.0V MosFet free run](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/analytic/7.0V_MosFet_FreeRun.png) | ![7.6V L298 free run](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/analytic/7.6V_L298_FreeRun.png) |
| Here you see that the speed is proportional to the PWM, but the minimal power to start the motor is 33/255 = 13% PWM or 0.9 volt. | Due to losses and other effects in the L298 the start voltage is much higher. |
| | |
| **MosFet bridge supplied by 3.5 volt** | **Start diagram for L298 with 6.2 volt** |
| ![3.5V MosFet free run](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/analytic/3.5V_MosFet_FreeRun.png) | ![7.6V L298 free run](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/analytic/6.2V_L298_FreeRun.png) |


## TestMotorWithIMU
| Diagram for car controlled by an MosFet bridge | Diagram for car controlled by an L298 bridge |
| :-: | :-: |
| ![2WD Smart Car](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/analytic/2WD_Test.png) | ![Lafvin car](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/analytic/Lafvin_Test.png) |

## RobotCarBasic
Template for your RobotCar control. Currently implemented is: drive until distance too low, then stop, and turn random amount.

## RobotCarFollowerSimple
The car tries to hold a distance between 20 and 30 cm to an obstacle. The measured distance is converted to a pitch as an acoustic feedback.

## RobotCarFollower
The car tries to hold a distance between 20 and 30 cm to an target. If the target vanishes, the distance sensor scans for the vanished or a new target.

## [RobotCarBlueDisplay](https://github.com/ArminJo/Arduino-RobotCar)
Enables autonomous driving of a 2 or 4 wheel car with an Arduino and a Adafruit Motor Shield V2.<br/>
To avoid obstacles a HC-SR04 Ultrasonic sensor mounted on a SG90 Servo continuously scans the environment.
Manual control is implemented by a GUI using a Bluetooth HC-05 Module and the BlueDisplay library.

Just overwrite the function doUserCollisionDetection() to test your own skill.<br/>
You may also overwrite the function fillAndShowForwardDistancesInfo(), if you use your own scanning method.

# Compile options / macros for RobotCar example
To customize the RobotCar example to cover different extensions, there are some compile options available.
| Option | Default | File | Description |
|-|-|-|-|
| `CAR_HAS_4_WHEELS` | disabled | RobotCar.h | Use modified formula for turning the car. |
| `US_SENSOR_SUPPORTS_1_PIN_MODE` | disabled | RobotCar.h | Use modified HC-SR04 modules or HY-SRF05 ones.</br>Modify HC-SR04 by connecting 10kOhm between echo and trigger and then use only trigger pin. |
| `CAR_HAS_IR_DISTANCE_SENSOR` | disabled | RobotCar.h | Use Sharp GP2Y0A21YK / 1080 IR distance sensor. |
| `CAR_HAS_TOF_DISTANCE_SENSOR` | disabled | RobotCar.h | Use VL53L1X TimeOfFlight distance sensor. |
| `DISTANCE_SERVO_IS_MOUNTED_HEAD_DOWN` | disabled | Distance.h | The distance servo is mounted head down to detect even small obstacles. |
| `CAR_HAS_CAMERA` | disabled | RobotCar.h | Enables the `Camera` button for the `PIN_CAMERA_SUPPLY_CONTROL` pin. |
| `CAR_HAS_LASER` | disabled | RobotCar.h | Enables the `Laser` button for the `PIN_LASER_OUT` / `LED_BUILTIN` pin. |
| `CAR_HAS_PAN_SERVO` | disabled | RobotCar.h | Enables the pan slider for the `PanServo` at the `PIN_PAN_SERVO` pin. |
| `CAR_HAS_TILT_SERVO` | disabled | RobotCar.h | Enables the tilt slider for the `TiltServo` at the `PIN_TILT_SERVO` pin.. |
| `MONITOR_LIPO_VOLTAGE` | disabled | RobotCar.h | Shows VIN voltage and monitors it for undervoltage. Requires 2 additional resistors at pin A2. |
| `VIN_VOLTAGE_CORRECTION` | undefined | RobotCar.h | Voltage to be subtracted from VIN voltage. E.g. if there is a series diode between LIPO and VIN set it to 0.8. |

#### If you find this library useful, please give it a star.

# Pictures
Connection schematic of the L298 board for the examples. If motor drives in opposite direction, you must flip the motor to L298 connections.
![L298 connections](https://github.com/ArminJo/PWMMotorControl/blob/master/extras/L298_Connections_Steckplatine.png)
Connections on the Arduino and on the L298 board.<br/>
![Sensor shield connections](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/L298CarConnections_small.jpg)
![L298 connections](https://github.com/ArminJo/PWMMotorControl/blob/master/pictures/L298Connections_small.jpg)

2 wheel car with encoders, slot-type photo interrupter, 2 LiPo batteries, Adafruit Motor Shield V2, HC-05 Bluetooth module, and servo mounted head down.
![2 wheel car](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/2WheelDriveCar.jpg)
4 wheel car with servo mounted head up.
![4 wheel car](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/4WheelDriveCar.jpg)
Encoder slot-type photo interrupter sensor
![Encoder slot-type photo interrupter sensor](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/ForkSensor.jpg)
Servo mounted head down
![Servo mounting](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/ServoAtTopBack.jpg)
VIN sensing
![VIN sensing](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/SensingVIn.jpg)

# SCREENSHOTS
Start page
![MStart page](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/HomePage.png)
Test page
![Manual control page](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/TestPage.png)
Automatic control page with detected wall at right
![Automatic control page](https://github.com/ArminJo/Arduino-RobotCar/blob/master/pictures/AutoDrivePage.png)
- Green bars are distances above 1 meter or above double distance of one ride per scan whichever is less.
- Red bars are distanced below the distance of one ride per scan -> collision during next "scan and ride" cycle if obstacle is ahead.
- Orange bars are the values between the 2 thresholds.
- The tiny white bars are the distances computed by the doWallDetection() function. They overlay the green (assumed timeout) values.
- The tiny black bar is the rotation chosen by doCollisionDetection() function.

# Revision History
### Version 2.0.0 - work in progress
- Removed all *Compensated functions, compensation now is always active.
- Removed StopSpeed from EepromMotorinfoStruct.
- Removed StartSpeed.
- Renamed *.cpp to *.hpp.
- Added and renamed functions.
- IMU / MPU6050 support.
- Support of off the shelf smart cars.
- Added and renamed functions.
- Converted to voltage based formulas.

### Version 1.0.0
- Initial Arduino library version.

