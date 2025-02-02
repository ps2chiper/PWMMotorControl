/*
 *  TestMotorWithIMU.cpp
 *  Tests Prints PWM, distance and speed diagram of an encoder motor.
 *  Encoder and IMU data are printed simultaneously, to compare and to detect slipping
 *
 *
 *  Copyright (C) 2020-2021  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-RobotCar https://github.com/ArminJo/PWMMotorControl.
 *
 *  PWMMotorControl is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

/*
 * You will need to change these values according to your motor, H-bridge and motor supply voltage.
 * You must specify this before the include of "CarPWMMotorControl.hpp"
 */
#define USE_ENCODER_MOTOR_CONTROL  // Activate this if you have encoder interrupts attached at pin 2 and 3 and want to use the methods of the EncoderMotor class.
#define USE_MPU6050_IMU  // Activate this if you use GY-521 MPU6050 breakout board for precise turning and speed / distance movement. Connectors point to the rear.
//#define USE_ADAFRUIT_MOTOR_SHIELD  // Activate this if you use Adafruit Motor Shield v2 connected by I2C instead of TB6612 or L298 breakout board.
//#define USE_STANDARD_LIBRARY_FOR_ADAFRUIT_MOTOR_SHIELD  // Activate this to force using of Adafruit library. Requires 694 bytes program memory.
//#define VIN_2_LIPO                 // Activate this, if you use 2 LiPo Cells (around 7.4 volt) as Motor supply.
//#define VIN_1_LIPO                 // Or if you use a Mosfet bridge, 1 LIPO (around 3.7 volt) may be sufficient.
//#define FULL_BRIDGE_INPUT_MILLIVOLT   6000  // Default. For 4 x AA batteries (6 volt).
//#define MOSFET_BRIDGE_USED  // Activate this, if you use a (recommended) mosfet bridge instead of a L298 bridge, which has higher losses.
//#define DEFAULT_DRIVE_MILLIVOLT       2000 // Drive voltage -motors default speed- is 2.0 volt
//#define DO_NOT_SUPPORT_RAMP  // Ramps are anyway not used if drive speed voltage (default 2.0 V) is below 2.3 V. Saves 378 bytes program space.
#include "CarPWMMotorControl.hpp"

#include "IMUCarData.hpp"

#include "RobotCarPinDefinitionsAndMore.h"

#define ONLY_ARDUINO_PLOTTER_OUTPUT
#define PRINTS_PER_SECOND 50

CarPWMMotorControl CarPWMMotorControl;

unsigned long LastPrintMillis;

void printData(uint8_t aDataSetsToPrint, uint16_t aPeriodMillis, bool aUseRamp);

void setup() {
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);

#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) || defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
    // Just to know which program is running on my Arduino
#ifndef ONLY_ARDUINO_PLOTTER_OUTPUT
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_PWMMOTORCONTROL));

    // Print info
    PWMDcMotor::printSettings(&Serial);
#endif

#ifdef USE_ADAFRUIT_MOTOR_SHIELD
    // For Adafruit Motor Shield v2
    CarPWMMotorControl.init();
#else
    CarPWMMotorControl.init(RIGHT_MOTOR_FORWARD_PIN, RIGHT_MOTOR_BACKWARD_PIN, RIGHT_MOTOR_PWM_PIN, LEFT_MOTOR_FORWARD_PIN,
    LEFT_MOTOR_BACKWARD_PIN, LEFT_MOTOR_PWM_PIN);
#endif

    CarPWMMotorControl.IMUData.delayAndReadIMUCarDataData(3000);
}

void loop() {
    static uint8_t sDirection = DIRECTION_FORWARD;

#if defined(USE_ENCODER_MOTOR_CONTROL)
    CarPWMMotorControl.rightCarMotor.printEncoderDataCaption(&Serial);
#endif
    CarPWMMotorControl.IMUData.printIMUCarDataCaption(&Serial);
#if ! defined(USE_ENCODER_MOTOR_CONTROL)
    Serial.print(F("PWM[2] "));
#endif
    Serial.print(
            F(
                    STR(PRINTS_PER_SECOND) "_values/s_at_" STR(SAMPLE_RATE) "_samples/s_offset=" STR(RAMP_VALUE_OFFSET_MILLIVOLT) "_mV_delta="));
    Serial.print(RAMP_VALUE_DELTA);
    Serial.print(F("_decel="));
    Serial.print(RAMP_DECELERATION_TIMES_2 / 2);
    Serial.println(F("mm/s^2"));

    /*
     * Set to drive speed without ramp
     */
#ifndef ONLY_ARDUINO_PLOTTER_OUTPUT
    Serial.print(F("Set speed to DEFAULT_DRIVE_SPEED="));
    Serial.println(DEFAULT_DRIVE_SPEED);
#endif
    uint8_t sSpeedPWM = DEFAULT_DRIVE_SPEED_PWM;

    uint8_t tLoopIndex = 0;

    while (true) {
        bool tUseRamp = true;
        for (uint8_t i = 0; i < 2; ++i) {

            CarPWMMotorControl.IMUData.resetOffsetDataAndWait();
#if defined(USE_ENCODER_MOTOR_CONTROL)
            CarPWMMotorControl.rightCarMotor.resetEncoderControlValues();
#endif

            if (tUseRamp) {
#ifndef ONLY_ARDUINO_PLOTTER_OUTPUT
                Serial.print(F("Go distance[mm]="));
                Serial.println((tLoopIndex + 1) * 200); // 200, 400, 600
#endif
                CarPWMMotorControl.startGoDistanceMillimeter(sSpeedPWM, (tLoopIndex + 1) * 200, sDirection);
                Serial.print(F("Go distance[mm]="));
                Serial.println(CarPWMMotorControl.CarRequestedDistanceMillimeter);
                // print 20 data sets after stopping
                printData(40, 1000 / PRINTS_PER_SECOND, tUseRamp);
            } else {
                CarPWMMotorControl.setSpeedPWM(sSpeedPWM, sDirection);
                printData(40, 1000 / PRINTS_PER_SECOND, tUseRamp);
#ifndef ONLY_ARDUINO_PLOTTER_OUTPUT
                Serial.println(F("Stop motors"));
#endif
                CarPWMMotorControl.setStopMode(MOTOR_BRAKE); // just to be sure
                CarPWMMotorControl.setSpeedPWM(0);
                printData(20, 1000 / PRINTS_PER_SECOND, tUseRamp);
            }

            tUseRamp = false;
            CarPWMMotorControl.IMUData.delayAndReadIMUCarDataData(1000);

        }
        if (sSpeedPWM == MAX_SPEED_PWM) {
            break; // after last loop
        }
        // double speed for next turn
        if (sSpeedPWM <= MAX_SPEED_PWM / 2) {
            sSpeedPWM *= 2;
        } else {
            // last loop with MAX_SPEED
            sSpeedPWM = MAX_SPEED_PWM;
        }
        tLoopIndex++;

#ifndef ONLY_ARDUINO_PLOTTER_OUTPUT
        Serial.print(F("Set speed to:"));
        Serial.println(sSpeedPWM);
#endif
        CarPWMMotorControl.IMUData.delayAndReadIMUCarDataData(1000);

    }
    CarPWMMotorControl.IMUData.delayAndReadIMUCarDataData(5000);
    /*
     * switch direction
     */
    sDirection = oppositeDIRECTION(sDirection);
#ifndef ONLY_ARDUINO_PLOTTER_OUTPUT
    Serial.print(F("Switch direction to:"));
    Serial.println(sDirection);
#endif
}

/*
 * Prints values, if a new value is available
 * @return true if printed.
 */
void printData(uint8_t aDataSetsToPrint, uint16_t aPeriodMillis, bool aUseRamp) {

    for (uint_fast8_t i = 0; i < aDataSetsToPrint;) {
        if (aUseRamp) {
            if (CarPWMMotorControl.updateMotors()) {
                // do not count as long as car is driving
                i = 0;
            }
        }
#if defined(USE_ENCODER_MOTOR_CONTROL)

        if (CarPWMMotorControl.rightCarMotor.printEncoderDataPeriodically(&Serial, aPeriodMillis)) {
            CarPWMMotorControl.IMUData.readCarDataFromMPU6050Fifo();
            CarPWMMotorControl.IMUData.printIMUCarData(&Serial);
            Serial.println();
            i++;
        }
#else
        if (CarPWMMotorControl.IMUData.printIMUCarDataDataPeriodically(&Serial, aPeriodMillis)) {
            Serial.println(CarPWMMotorControl.rightCarMotor.CurrentSpeedPWM / 2); // = PWM, scale it for plotter
            i++;
        }
#endif
    }
}
