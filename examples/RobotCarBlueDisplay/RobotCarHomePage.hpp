/*
 * RobotCarHomePage.hpp
 *
 *  Contains the GUI elements of the home page of RobotCarPWMMotorControl.
 *
 *  Requires BlueDisplay library.
 *
 *  Copyright (C) 2016-2022  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-RobotCar https://github.com/ArminJo/Arduino-RobotCar.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef ROBOT_CAR_HOME_PAGE_HPP
#define ROBOT_CAR_HOME_PAGE_HPP

#include <Arduino.h>

#include "RobotCarPinDefinitionsAndMore.h"
#include "RobotCarBlueDisplay.h"

#include "RobotCarGui.h"
#include "Distance.h"

BDButton TouchButtonTestPage;
BDButton TouchButtonBTSensorDrivePage;
BDButton TouchButtonLaser;
#ifdef CAR_ENABLE_RTTTL
BDButton TouchButtonMelody;
#endif
#ifdef CAR_HAS_CAMERA
BDButton TouchButtonCameraOnOff;
#endif

#ifdef CAR_HAS_PAN_SERVO
BDSlider SliderPan;
#endif
#ifdef CAR_HAS_TILT_SERVO
BDSlider SliderTilt;
#endif

#pragma GCC diagnostic ignored "-Wunused-parameter"

// Here we get values from 0 to 180 degrees from scaled slider
#ifdef CAR_HAS_PAN_SERVO
void doHorizontalServoPosition(BDSlider *aTheTouchedSlider, uint16_t aValue) {
    PanServo.write(aValue);
}
#endif

#ifdef CAR_HAS_TILT_SERVO
void doVerticalServoPosition(BDSlider *aTheTouchedSlider, uint16_t aValue) {
    TiltServo.write(aValue);
}
#endif

#ifdef LASER_MOUNTED
void doLaserOnOff(BDButton * aTheTouchedButton, int16_t aValue) {
    digitalWrite(PIN_LASER_OUT, aValue);
}
#endif

#ifdef CAR_HAS_CAMERA
void doCameraSupplyOnOff(BDButton * aTheTouchedButton, int16_t aValue) {
    digitalWrite(PIN_CAMERA_SUPPLY_CONTROL, aValue);
}
#endif

#ifdef CAR_ENABLE_RTTTL
void doPlayMelody(BDButton * aTheTouchedButton, int16_t aValue) {
    sPlayMelody = aValue;
}
#endif

void initHomePage(void) {

    TouchButtonBTSensorDrivePage.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED,
            F("Sensor\nDrive"), TEXT_SIZE_18, FLAG_BUTTON_DO_BEEP_ON_TOUCH, PAGE_BT_SENSOR_CONTROL, &GUISwitchPages);

    TouchButtonAutomaticDrivePage.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED,
            F("Automatic\nControl"), TEXT_SIZE_16, FLAG_BUTTON_DO_BEEP_ON_TOUCH, PAGE_AUTOMATIC_CONTROL, &GUISwitchPages);

    TouchButtonTestPage.init(BUTTON_WIDTH_3_POS_3,
    BUTTON_HEIGHT_4_LINE_4 - (TEXT_SIZE_22_HEIGHT + BUTTON_DEFAULT_SPACING_QUARTER), BUTTON_WIDTH_3, TEXT_SIZE_22_HEIGHT, COLOR16_RED,
            F("Test"), TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, PAGE_TEST, &GUISwitchPages);

#ifdef CAR_HAS_CAMERA
    TouchButtonCameraOnOff.init(BUTTON_WIDTH_8_POS_4, BUTTON_HEIGHT_8_LINE_3, BUTTON_WIDTH_8,
    TEXT_SIZE_22_HEIGHT, COLOR16_BLACK, F("Cam"), TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN,
            false, &doCameraSupplyOnOff);
#endif

#ifdef CAR_ENABLE_RTTTL
    TouchButtonMelody.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4 - (TEXT_SIZE_22_HEIGHT + BUTTON_DEFAULT_SPACING_QUARTER),
    BUTTON_WIDTH_3, BUTTON_HEIGHT_8, COLOR16_BLACK, F("Melody"), TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sPlayMelody, &doPlayMelody);
#endif

#ifdef LASER_MOUNTED
    TouchButtonLaser.init(0, BUTTON_HEIGHT_4_LINE_4 - (TEXT_SIZE_22_HEIGHT + BUTTON_DEFAULT_SPACING_QUARTER),
    BUTTON_WIDTH_3, TEXT_SIZE_22_HEIGHT, COLOR16_BLACK, F("Laser"), TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, false, &doLaserOnOff);
#endif

}

/*
 * Manual control page
 */
void drawHomePage(void) {
    drawCommonGui();
    BlueDisplay1.drawText(HEADER_X + TEXT_SIZE_22_WIDTH, (2 * TEXT_SIZE_22_HEIGHT), F("Control"));

#if defined(CAR_HAS_4_WHEELS)
    char tCarTypeString[] = "4WD";
#else
    char tCarTypeString[] = "2WD";
#endif
#ifdef CAR_HAS_CAMERA
    BlueDisplay1.drawText(HEADER_X + (2 * TEXT_SIZE_22_WIDTH), (2 * TEXT_SIZE_22_HEIGHT) + TEXT_SIZE_11_HEIGHT - 2, tCarTypeString,
            TEXT_SIZE_11, COLOR16_RED, COLOR16_NO_BACKGROUND);
#else
    BlueDisplay1.drawText(HEADER_X + (2 * TEXT_SIZE_22_WIDTH), (3 * TEXT_SIZE_22_HEIGHT), tCarTypeString);
#endif

    TouchButtonRobotCarStartStop.drawButton();
#ifdef CAR_HAS_CAMERA
    TouchButtonCameraOnOff.drawButton();
#endif
#ifdef CAR_ENABLE_RTTTL
    TouchButtonMelody.drawButton();
#endif
#ifdef LASER_MOUNTED
    TouchButtonLaser.drawButton();
#endif
    TouchButtonBTSensorDrivePage.drawButton();
    TouchButtonTestPage.drawButton();
    TouchButtonAutomaticDrivePage.drawButton();

    TouchButtonDirection.drawButton();
#if defined(USE_ENCODER_MOTOR_CONTROL) || defined(USE_MPU6050_IMU)
    TouchButtonCalibrate.drawButton();
#endif
    TouchButtonCompensationLeft.drawButton();
    TouchButtonCompensationRight.drawButton();
#ifdef ENABLE_EEPROM_STORAGE
    TouchButtonCompensationStore.drawButton();
#endif

//    SliderUSPosition.setValueAndDrawBar(sLastServoAngleInDegrees);
    SliderUSPosition.drawSlider();
    SliderUSDistance.drawSlider();

#if defined(CAR_HAS_IR_DISTANCE_SENSOR) || defined(CAR_CAR_HAS_TOF_DISTANCE_SENSOR) && ( ! (defined(CAR_HAS_PAN_SERVO) && defined(CAR_HAS_TILT_SERVO)))
    SliderIROrTofDistance.drawSlider();
#endif

#ifdef CAR_HAS_PAN_SERVO
    SliderPan.drawSlider();
#endif
#ifdef CAR_HAS_TILT_SERVO
    SliderTilt.drawSlider();
#endif

    SliderSpeed.drawSlider();
#ifdef USE_ENCODER_MOTOR_CONTROL
    SliderSpeedRight.drawSlider();
    SliderSpeedLeft.drawSlider();
#endif
    PWMDcMotor::MotorControlValuesHaveChanged = true; // trigger drawing of values
}

void startHomePage(void) {
    TouchButtonDirection.setPosition(BUTTON_WIDTH_8_POS_5, BUTTON_HEIGHT_8_LINE_5);
#if defined(USE_ENCODER_MOTOR_CONTROL) || defined(USE_MPU6050_IMU)
    TouchButtonCalibrate.setPosition(BUTTON_WIDTH_8_POS_5, BUTTON_HEIGHT_8_LINE_3);
#endif
#if defined(CAR_HAS_TILT_SERVO) && defined(ENABLE_EEPROM_STORAGE)
    TouchButtonCompensationStore.setPosition(BUTTON_WIDTH_8_POS_4, BUTTON_HEIGHT_8_LINE_5);
#endif
    drawHomePage();
}

void loopHomePage(void) {
}

void stopHomePage(void) {
    TouchButtonDirection.setPosition(BUTTON_WIDTH_8_POS_6, BUTTON_HEIGHT_8_LINE_6);
#if defined(USE_ENCODER_MOTOR_CONTROL) || defined(USE_MPU6050_IMU)
    TouchButtonCalibrate.setPosition(BUTTON_WIDTH_8_POS_6, BUTTON_HEIGHT_8_LINE_2);
#endif
#if defined(CAR_HAS_TILT_SERVO) && defined(ENABLE_EEPROM_STORAGE)
    TouchButtonCompensationStore.setPosition(BUTTON_WIDTH_8_POS_6, BUTTON_HEIGHT_8_LINE_4);
#endif
    startStopRobotCar(false);
}
#endif // ROBOT_CAR_HOME_PAGE_HPP
#pragma once

