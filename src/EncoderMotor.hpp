/*
 * EncoderMotor.hpp
 *
 *  Functions for controlling a DC-motor which rotary encoder implemented by slot-type photo interrupter and an attached encoder disc (with 20 slots).
 *  Works with positive (unsigned) speed and direction or signed speed.
 *
 *  Contains functions to go a specified distance.
 *  These functions generates ramps for acceleration and deceleration and tries to stop at target distance.
 *  This enables deterministic turns for 2-Wheel Cars.  For 4-Wheel cars it is impossible
 *  to get deterministic turns, therefore I use approximated thumb values.
 *
 *  Tested for Adafruit Motor Shield and plain TB6612 breakout board.
 *
 *  Created on: 12.05.2019
 *  Copyright (C) 2019-2021  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of PWMMotorControl https://github.com/ArminJo/PWMMotorControl.
 *
 *  PWMMotorControl is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 */
#ifndef ENCODER_MOTOR_CONTROL_HPP
#define ENCODER_MOTOR_CONTROL_HPP

#include <Arduino.h>
#if defined(USE_ENCODER_MOTOR_CONTROL)
#include "EncoderMotor.h"
#include "PWMDcMotor.hpp"

//#define TRACE
//#define DEBUG

EncoderMotor *sPointerForInt0ISR;
EncoderMotor *sPointerForInt1ISR;

EncoderMotor::EncoderMotor() : // @suppress("Class members should be properly initialized")
                               PWMDcMotor(), stopFlag(false)
{
#ifdef ENABLE_MOTOR_LIST_FUNCTIONS
    AddToMotorList();
#endif
}

#ifdef USE_ADAFRUIT_MOTOR_SHIELD
/*
 * aMotorNumber from 1 to 2
 * If no parameter, we use a fixed assignment of rightCarMotor interrupts to INT0 / Pin2 and leftCarMotor to INT1 / Pin3
 * Currently motors 3 and 4 are not required/supported by own library for Adafruit Motor Shield
 */
void EncoderMotor::init(uint8_t aMotorNumber)
{
    PWMDcMotor::init(aMotorNumber); // create with the default frequency 1.6KHz
    resetEncoderControlValues();
}
void EncoderMotor::init(uint8_t aMotorNumber, uint8_t aInterruptNumber)
{
    PWMDcMotor::init(aMotorNumber); // create with the default frequency 1.6KHz
    resetEncoderControlValues();
    attachInterrupt(aInterruptNumber);
}
#else
EncoderMotor::EncoderMotor(uint8_t aForwardPin, uint8_t aBackwardPin, uint8_t aPWMPin) : // @suppress("Class members should be properly initialized")
                                                                                         PWMDcMotor(aForwardPin, aBackwardPin, aPWMPin)
{
    resetEncoderControlValues();
#ifdef ENABLE_MOTOR_LIST_FUNCTIONS
    AddToMotorList();
#endif
}

void EncoderMotor::init(uint8_t aForwardPin, uint8_t aBackwardPin, uint8_t aPWMPin)
{
    PWMDcMotor::init(aForwardPin, aBackwardPin, aPWMPin);
    resetEncoderControlValues();
}

void EncoderMotor::init(uint8_t aForwardPin, uint8_t aBackwardPin, uint8_t aPWMPin, uint8_t aInterruptNumber)
{
    PWMDcMotor::init(aForwardPin, aBackwardPin, aPWMPin);
    resetEncoderControlValues();
    attachEncoderInterrupt(aInterruptNumber);
}
#endif

/*
 * If motor is already running, adjust TargetDistanceCount to go to aRequestedDistanceMillimeter
 */
void EncoderMotor::startGoDistanceMillimeter(uint8_t aRequestedSpeedPWM, unsigned int aRequestedDistanceMillimeter,
                                             uint8_t aRequestedDirection)
{
    if (aRequestedDistanceMillimeter == 0)
    {
        stop(DefaultStopMode); // In case motor was running
        return;
    }
    if (CurrentSpeedPWM == 0)
    {
        setSpeedPWMWithRamp(aRequestedSpeedPWM, aRequestedDirection);
        TargetDistanceMillimeter = aRequestedDistanceMillimeter;
    }
    else
    {
        /*
         * Already moving
         */
        MotorRampState = MOTOR_STATE_DRIVE; // must be set, since we may be moving until now without ramp control
        TargetDistanceMillimeter = getDistanceMillimeter() + aRequestedDistanceMillimeter;
        PWMDcMotor::setSpeedPWM(aRequestedSpeedPWM, aRequestedDirection);
    }
    LastTargetDistanceMillimeter = TargetDistanceMillimeter;
    CheckDistanceInUpdateMotor = true;
}

void EncoderMotor::startGoDistanceMillimeter(unsigned int aRequestedDistanceMillimeter, uint8_t aRequestedDirection)
{
    startGoDistanceMillimeter(DriveSpeedPWM, aRequestedDistanceMillimeter, aRequestedDirection);
}

/*
 * if aRequestedDistanceMillimeter < 0 then use DIRECTION_BACKWARD
 * initialize motorInfo fields DirectionForward, RequestedDriveSpeedPWM, DistanceTickCounter and optional NextChangeMaxTargetCount.
 */
void EncoderMotor::startGoDistanceMillimeter(int aRequestedDistanceMillimeter)
{
    if (aRequestedDistanceMillimeter < 0)
    {
        aRequestedDistanceMillimeter = -aRequestedDistanceMillimeter;
        startGoDistanceMillimeter(aRequestedDistanceMillimeter, DIRECTION_BACKWARD);
    }
    else
    {
        startGoDistanceMillimeter(aRequestedDistanceMillimeter, DIRECTION_FORWARD);
    }
}

/*
 * @return true if not stopped (motor expects another update)
 */
bool EncoderMotor::updateMotor()
{
    unsigned long tMillis = millis();
    uint8_t tNewSpeedPWM = CurrentSpeedPWM;

    /*
     * Check if target distance is reached or encoder tick has timeout
     */
    if (tNewSpeedPWM > 0)
    {
        if (CheckDistanceInUpdateMotor && (getDistanceMillimeter() >= TargetDistanceMillimeter || tMillis > (LastEncoderInterruptMillis + ENCODER_SENSOR_TIMEOUT_MILLIS)))
        {
            stop(MOTOR_BRAKE); // this sets MOTOR_STATE_STOPPED;
#ifdef DEBUG
            Serial.print(PWMPin);
            if (tMillis > (LastEncoderInterruptMillis + ENCODER_SENSOR_TIMEOUT_MILLIS))
            {
                Serial.print(F(" Encoder timeout: dist="));
            }
            else
            {
                Serial.print(F(" Reached: dist="));
            }
            Serial.print(getDistanceMillimeter());
            Serial.print(F(" Breakdist="));
            Serial.println(getBrakingDistanceMillimeter());
#endif
            return false; // need no more calls to update()
        }
    }

    if (MotorRampState == MOTOR_STATE_START)
    {
        initEncoderControlValues();

        NextRampChangeMillis = tMillis + RAMP_INTERVAL_MILLIS;
        /*
         * Start motor
         */
        if (RequestedDriveSpeedPWM > RAMP_VALUE_OFFSET_SPEED_PWM)
        {
            // start with ramp to avoid spinning wheels
            tNewSpeedPWM = RAMP_VALUE_OFFSET_SPEED_PWM; // start immediately with speed offset (2.3 volt)
            //  --> RAMP_UP
            MotorRampState = MOTOR_STATE_RAMP_UP;
        }
        else
        {
            // Motor ramp not required, go direct to drive speed.
            tNewSpeedPWM = RequestedDriveSpeedPWM;
            //  --> DRIVE
            MotorRampState = MOTOR_STATE_DRIVE;
        }
    }
    else if (MotorRampState == MOTOR_STATE_RAMP_UP)
    {
        if (tMillis >= NextRampChangeMillis)
        {
            NextRampChangeMillis += RAMP_INTERVAL_MILLIS;
            /*
             * Increase motor speed by RAMP_VALUE_DELTA every RAMP_UPDATE_INTERVAL_MILLIS milliseconds
             * Transition criteria to next state is:
             * Drive speed reached or target distance - braking distance reached
             */
            if (tNewSpeedPWM == RequestedDriveSpeedPWM || (CheckDistanceInUpdateMotor && getDistanceMillimeter() + getBrakingDistanceMillimeter() >= TargetDistanceMillimeter))
            {
                //  RequestedDriveSpeedPWM reached switch to --> DRIVE_SPEED_PWM and check immediately for next transition to RAMP_DOWN
                MotorRampState = MOTOR_STATE_DRIVE;
            }
            else
            {
                tNewSpeedPWM = tNewSpeedPWM + RAMP_VALUE_DELTA;
                // Clip value and check for 8 bit overflow
                if (tNewSpeedPWM > RequestedDriveSpeedPWM || tNewSpeedPWM <= RAMP_VALUE_DELTA)
                {
                    // do not change state here to let motor run at RequestedDriveSpeedPWM for one interval
                    tNewSpeedPWM = RequestedDriveSpeedPWM;
                }
            }
#ifdef DEBUG
            Serial.print(PWMPin);
            Serial.print(F(" St="));
            Serial.print(MotorRampState);
            Serial.print(F(" Ns="));
            Serial.println(tNewSpeedPWM);
#endif
        }
    }

    // do not use "else if" since we must immediately check for next transition to RAMP_DOWN
    if (MotorRampState == MOTOR_STATE_DRIVE)
    {
        /*
         * Wait until target distance - braking distance reached
         */
        if (CheckDistanceInUpdateMotor && getDistanceMillimeter() + getBrakingDistanceMillimeter() >= TargetDistanceMillimeter)
        {
            if (CurrentSpeedPWM > RAMP_VALUE_OFFSET_SPEED_PWM)
            {
                tNewSpeedPWM -= (RAMP_VALUE_OFFSET_SPEED_PWM - RAMP_VALUE_DELTA); // RAMP_VALUE_DELTA is immediately subtracted below
            }
            else
            {
                tNewSpeedPWM = RAMP_VALUE_MIN_SPEED_PWM;
            }
            //  --> RAMP_DOWN
            MotorRampState = MOTOR_STATE_RAMP_DOWN;
#ifdef DEBUG
            Serial.print(PWMPin);
            Serial.print(F(" Dist="));
            Serial.print(getDistanceMillimeter());
            Serial.print(F(" Breakdist="));
            Serial.print(getBrakingDistanceMillimeter());
            Serial.print(F(" St="));
            Serial.print(MotorRampState);
            Serial.print(F(" Ns="));
            Serial.println(tNewSpeedPWM);
#endif
        }
    }

    // do not use "else if" since we must immediately check for next transition to STOPPED
    if (MotorRampState == MOTOR_STATE_RAMP_DOWN)
    {
        if (tMillis >= NextRampChangeMillis)
        {
            NextRampChangeMillis = tMillis + RAMP_INTERVAL_MILLIS;
            /*
             * Decrease motor speed RAMP_UPDATE_INTERVAL_STEPS times every RAMP_UPDATE_INTERVAL_MILLIS milliseconds
             */
            if (tNewSpeedPWM > (RAMP_VALUE_DELTA + RAMP_VALUE_MIN_SPEED_PWM))
            {
                tNewSpeedPWM -= RAMP_VALUE_DELTA;
            }
            else
            {
                tNewSpeedPWM = RAMP_VALUE_MIN_SPEED_PWM;
            }
#ifdef DEBUG
            Serial.print(PWMPin);
            Serial.print(F(" St="));
            Serial.print(MotorRampState);
            Serial.print(F(" Ns="));
            Serial.println(tNewSpeedPWM);
#endif
        }
    }
    // End of motor state machine

#ifdef TRACE
    Serial.print(PWMPin);
    Serial.print(F(" St="));
    Serial.println(MotorRampState);
#endif
    if (tNewSpeedPWM != CurrentSpeedPWM)
    {
#ifdef TRACE
        Serial.print(F("Ns="));
        Serial.println(tNewSpeedPWM);
#endif
        PWMDcMotor::setSpeedPWM(tNewSpeedPWM, CurrentDirectionOrBrakeMode);
    }
    return (CurrentSpeedPWM > 0); // current speed == 0
}

/*
 * Computes motor speed compensation value in order to go exactly straight ahead
 * Compensate only at forward direction
 */
void EncoderMotor::synchronizeMotor(EncoderMotor *aOtherMotorControl, unsigned int aCheckInterval)
{
    if (CurrentDirectionOrBrakeMode != DIRECTION_FORWARD || aOtherMotorControl->CurrentDirectionOrBrakeMode != DIRECTION_FORWARD)
    {
        return;
    }
    static long sNextMotorSyncMillis;
    long tMillis = millis();
    if (tMillis >= sNextMotorSyncMillis)
    {
        sNextMotorSyncMillis += aCheckInterval;
        // only synchronize if manually operated or at full speed
        if ((MotorRampState == MOTOR_STATE_STOPPED && aOtherMotorControl->MotorRampState == MOTOR_STATE_STOPPED && CurrentSpeedPWM > 0) || (MotorRampState == MOTOR_STATE_DRIVE && aOtherMotorControl->MotorRampState == MOTOR_STATE_DRIVE))
        {
            MotorControlValuesHaveChanged = false;
            if (EncoderCount >= (aOtherMotorControl->EncoderCount + 2))
            {
                EncoderCount = aOtherMotorControl->EncoderCount;
                /*
                 * This motor is too fast, first try to reduce other motors compensation
                 */
                if (aOtherMotorControl->SpeedPWMCompensation >= 2)
                {
                    aOtherMotorControl->SpeedPWMCompensation -= 2;
                    aOtherMotorControl->CurrentSpeedPWM += 2;
                    aOtherMotorControl->setSpeedPWM(aOtherMotorControl->CurrentSpeedPWM, DIRECTION_FORWARD);
                    MotorControlValuesHaveChanged = true;
                    EncoderCount = aOtherMotorControl->EncoderCount;
                }
                else if (CurrentSpeedPWM > DriveSpeedPWM / 2)
                {
                    /*
                     * else increase this motors compensation
                     */
                    SpeedPWMCompensation += 2;
                    CurrentSpeedPWM -= 2;
                    PWMDcMotor::setSpeedPWM(CurrentSpeedPWM, DIRECTION_FORWARD);
                    MotorControlValuesHaveChanged = true;
                }
            }
            else if (aOtherMotorControl->EncoderCount >= (EncoderCount + 2))
            {
                aOtherMotorControl->EncoderCount = EncoderCount;
                /*
                 * Other motor is too fast, first try to reduce this motors compensation
                 */
                if (SpeedPWMCompensation >= 2)
                {
                    SpeedPWMCompensation -= 2;
                    CurrentSpeedPWM += 2;
                    PWMDcMotor::setSpeedPWM(CurrentSpeedPWM, DIRECTION_FORWARD);
                    MotorControlValuesHaveChanged = true;
                }
                else if (aOtherMotorControl->CurrentSpeedPWM > aOtherMotorControl->DriveSpeedPWM / 2)
                {
                    /*
                     * else increase other motors compensation
                     */
                    aOtherMotorControl->SpeedPWMCompensation += 2;
                    aOtherMotorControl->CurrentSpeedPWM -= 2;
                    aOtherMotorControl->setSpeedPWM(aOtherMotorControl->CurrentSpeedPWM, DIRECTION_FORWARD);
                    MotorControlValuesHaveChanged = true;
                }
            }
        }
    }
}

void EncoderMotor::wheelGoDistanceTicks(int aRequestedDistanceTicks, uint8_t aRequestedSpeedPWM, uint8_t aRequestedDirection)
{
    stopFlag = true;
    startCount = 0;
    stopCount = aRequestedDistanceTicks;
    // EncoderCount;
    PWMDcMotor::setSpeedPWM(aRequestedSpeedPWM, aRequestedDirection);
}

/*************************
 * Direct motor control
 *************************/
/*
 * Reset all control values as distances, debug values etc. to 0x00
 */
void EncoderMotor::resetEncoderControlValues()
{
    memset(reinterpret_cast<uint8_t *>(&TargetDistanceMillimeter), 0,
           (((uint8_t *)&Debug) + sizeof(Debug)) - reinterpret_cast<uint8_t *>(&TargetDistanceMillimeter));
    // to force display of initial values
    SensorValuesHaveChanged = true;
}

void EncoderMotor::initEncoderControlValues()
{
    LastRideEncoderCount = 0;
    EncoderCount = 0;
    // initialize for timeout detection
    LastEncoderInterruptMillis = millis() - ENCODER_SENSOR_RING_MILLIS - 1;
}

/*
 * Reset EncoderInterruptDeltaMillis, EncoderInterruptMillisArray, MillisArrayIndex and AverageSpeedIsValid
 */
void EncoderMotor::resetSpeedValues()
{
#ifdef SUPPORT_AVERAGE_SPEED
    memset((void *)&EncoderInterruptDeltaMillis, 0,
           ((uint8_t *)&AverageSpeedIsValid + sizeof(AverageSpeedIsValid)) - (uint8_t *)&EncoderInterruptDeltaMillis);
#else
    EncoderInterruptDeltaMillis = 0;
#endif
}

uint8_t EncoderMotor::getDirection()
{
    return LastDirection;
}

unsigned int EncoderMotor::getDistanceMillimeter()
{
    return LastRideEncoderCount * FACTOR_COUNT_TO_MILLIMETER_INTEGER_DEFAULT;
}

unsigned int EncoderMotor::getDistanceCentimeter()
{
    return (LastRideEncoderCount * FACTOR_COUNT_TO_MILLIMETER_INTEGER_DEFAULT) / 10;
}

unsigned int EncoderMotor::getBrakingDistanceMillimeter()
{
    unsigned int tSpeedCmPerSecond = getSpeed();
    //    return (tSpeedCmPerSecond * tSpeedCmPerSecond * 100) / RAMP_DECELERATION_TIMES_2; // overflow!
    return (tSpeedCmPerSecond * tSpeedCmPerSecond) / (RAMP_DECELERATION_TIMES_2 / 100);
}

/*
 * Speed is in cm/s for a 20 slot encoder disc
 * Reset speed values after 1 second
 */
unsigned int EncoderMotor::getSpeed()
{
    if (millis() - LastEncoderInterruptMillis > 1000)
    {
        resetSpeedValues(); // Reset speed values after 1 second
    }
    unsigned long tEncoderInterruptDeltaMillis = EncoderInterruptDeltaMillis;
    if (tEncoderInterruptDeltaMillis == 0)
    {
        return 0;
    }
    return (SPEED_SCALE_VALUE / tEncoderInterruptDeltaMillis);
}

#ifdef SUPPORT_AVERAGE_SPEED
/*
 * Speed is in cm/s for a 20 slot encoder disc
 * Average is computed over the full revolution to compensate for unequal distances of the laser cut encoder discs.
 * If we do not have 21 timestamps, average is computed over the existing ones
 */
unsigned int EncoderMotor::getAverageSpeed()
{
    int tAverageSpeed = 0;
    /*
     * First check for timeout
     */
    if (millis() - LastEncoderInterruptMillis > 1000)
    {
        resetSpeedValues(); // Reset speed values after 1 second
    }
    else
    {
        int8_t tMillisArrayIndex = MillisArrayIndex;
        if (!AverageSpeedIsValid)
        {
            tMillisArrayIndex--;
            if (tMillisArrayIndex > 0)
            {
                // here MillisArray is not completely filled and MillisArrayIndex had no wrap around
                tAverageSpeed = (SPEED_SCALE_VALUE * tMillisArrayIndex) / (EncoderInterruptMillisArray[tMillisArrayIndex] - EncoderInterruptMillisArray[0]);
            }
        }
        else
        {
            // tMillisArrayIndex points to the next value to write == the oldest value to overwrite
            unsigned long tOldestEncoderInterruptMillis = EncoderInterruptMillisArray[tMillisArrayIndex];

            // get index of current value
            tMillisArrayIndex--;
            if (tMillisArrayIndex < 0)
            {
                // wrap around
                tMillisArrayIndex = AVERAGE_SPEED_BUFFER_SIZE - 1;
            }
            tAverageSpeed = (SPEED_SCALE_VALUE * AVERAGE_SPEED_SAMPLE_SIZE) / (EncoderInterruptMillisArray[tMillisArrayIndex] - tOldestEncoderInterruptMillis);
        }
    }
    return tAverageSpeed;
}

/*
 * @param aLengthOfAverage only values from 1 to 20 are valid!
 */
unsigned int EncoderMotor::getAverageSpeed(uint8_t aLengthOfAverage)
{
    if (!AverageSpeedIsValid && MillisArrayIndex < aLengthOfAverage)
    {
        // cannot compute requested average
        return 0;
    }
    // get index of aLengthOfAverage counts before
    int8_t tHistoricIndex = (MillisArrayIndex - 1) - aLengthOfAverage;
    if (tHistoricIndex < 0)
    {
        // wrap around
        tHistoricIndex += AVERAGE_SPEED_BUFFER_SIZE;
    }
    int tAverageSpeed = (SPEED_SCALE_VALUE * aLengthOfAverage) / (LastEncoderInterruptMillis - EncoderInterruptMillisArray[tHistoricIndex]);

    return tAverageSpeed;
}
#endif

/*
 * Print caption for Serial Plotter
 * Space but NO println() at the end, to enable additional information to be printed
 */
void EncoderMotor::printEncoderDataCaption(Print *aSerial)
{
    //    aSerial->print(F("PWM[2] Speed[cm/s] SpeedAvg.Of10[cm/s] Count[22cm/20LSB] "));
    aSerial->print(F("PWM[2] Speed[cm/s] Distance[cm] "));
}

bool EncoderMotor::printEncoderDataPeriodically(Print *aSerial, uint16_t aPeriodMillis)
{
    static unsigned long sLastPrintMillis;

    if ((millis() - sLastPrintMillis) >= aPeriodMillis)
    { // print data every n ms
        sLastPrintMillis = millis();
        printEncoderData(aSerial);
        return true;
    }
    return false;
}

void EncoderMotor::printEncoderData(Print *aSerial)
{
    aSerial->print(CurrentSpeedPWM / 2); // = PWM, scale it for plotter
    aSerial->print(" ");
    aSerial->print(getSpeed());
    aSerial->print(" ");
#ifdef SUPPORT_AVERAGE_SPEED
//    aSerial->print(getAverageSpeed(10));
//    aSerial->print(" ");
#endif
    aSerial->print(getDistanceCentimeter());
    aSerial->print(" ");
}

#if defined ESP32
void IRAM_ATTR EncoderMotor::handleEncoderInterrupt()
{
#else
void EncoderMotor::handleEncoderInterrupt()
{
    Serial.print("handleEncoderInterrupt");
#endif
    long tMillis = millis();
    unsigned long tDeltaMillis = tMillis - LastEncoderInterruptMillis;
    if (tDeltaMillis <= ENCODER_SENSOR_RING_MILLIS)
    {
        // assume signal is ringing and do nothing
    }
    else
    {
        LastEncoderInterruptMillis = tMillis;
#ifdef SUPPORT_AVERAGE_SPEED
        uint8_t tMillisArrayIndex = MillisArrayIndex;
#endif
        if (tDeltaMillis < ENCODER_SENSOR_TIMEOUT_MILLIS)
        {
            EncoderInterruptDeltaMillis = tDeltaMillis;
        }
        else
        {
            // timeout
            EncoderInterruptDeltaMillis = 0;
#ifdef SUPPORT_AVERAGE_SPEED
            tMillisArrayIndex = 0;
            AverageSpeedIsValid = false;
#endif
        }
#ifdef SUPPORT_AVERAGE_SPEED
        EncoderInterruptMillisArray[tMillisArrayIndex++] = tMillis;
        if (tMillisArrayIndex >= AVERAGE_SPEED_BUFFER_SIZE)
        {
            tMillisArrayIndex = 0;
            AverageSpeedIsValid = true;
        }
        MillisArrayIndex = tMillisArrayIndex;
#endif

        EncoderCount++;
        LastRideEncoderCount++;
        SensorValuesHaveChanged = true;
    }
}

/******************************************************************************************
 * Static methods
 *****************************************************************************************/
/*
 * Enable both interrupts INT0/D2 or INT1/D3
 */
void EncoderMotor::attachEncoderInterrupt(uint8_t aInterruptNumber)
{
    attachEncoderInterrupt(digitalPinToInterrupt(aInterruptNumber), this);
}

/***************************************************
 * Encoder functions
 ***************************************************/
/*
 * Attaches INT0 or INT1 interrupt to this EncoderMotor
 * Interrupt is enabled on rising edges
 * We can not use both edges since the on and off times of the opto interrupter are too different
 * aInterruptNumber can be one of INT0 (at pin D2) or INT1 (at pin D3) for Atmega328
 */
void EncoderMotor::attachEncoderInterrupt(uint8_t aInterruptNumber, EncoderMotor *ptr)
{
    Serial.print("attachEncoderInterrupt: ");
    Serial.println(aInterruptNumber);

    if (aInterruptNumber == RIGHT_MOTOR_INTERRUPT)
    {
        sPointerForInt0ISR = ptr;
        attachInterrupt(aInterruptNumber, ISR0, RISING);
    }
    else if (aInterruptNumber == LEFT_MOTOR_INTERRUPT)
    {
        sPointerForInt1ISR = ptr;
        attachInterrupt(aInterruptNumber, ISR1, RISING);
    }
}

void EncoderMotor::ISR0()
{
    Serial.print("Right Interrupt: ");
    sPointerForInt0ISR->handleEncoderInterrupt();
    Serial.println(sPointerForInt0ISR->startCount);
    sPointerForInt0ISR->startCount++;
     if (sPointerForInt0ISR->stopFlag && sPointerForInt0ISR->startCount >= sPointerForInt0ISR->stopCount)
    {
        sPointerForInt0ISR->stopFlag = false;
        Serial.println("Right Wheel Stop!");
        sPointerForInt0ISR->stop(MOTOR_BRAKE);
    }
}

void EncoderMotor::ISR1()
{
    Serial.print("Left Interrupt: ");
    sPointerForInt1ISR->handleEncoderInterrupt();
    Serial.println(sPointerForInt1ISR->startCount);
    sPointerForInt1ISR->startCount++;
    if (sPointerForInt1ISR->stopFlag && sPointerForInt1ISR->startCount >= sPointerForInt1ISR->stopCount)
    {
        sPointerForInt1ISR->stopFlag = false;
        Serial.println("Left Wheel Stop!");
        sPointerForInt1ISR->stop(MOTOR_BRAKE);
    }
}

#ifdef ENABLE_MOTOR_LIST_FUNCTIONS
/*
 * The list version saves 100 bytes and is more flexible, compared with the array version
 */
uint8_t EncoderMotor::sNumberOfMotorControls = 0;
EncoderMotor *EncoderMotor::sMotorControlListStart = NULL;

void EncoderMotor::AddToMotorList()
{
    EncoderMotor::sNumberOfMotorControls++;
    NextMotorControl = NULL;
    if (sMotorControlListStart == NULL)
    {
        // first constructor
        sMotorControlListStart = this;
    }
    else
    {
        // put object in control list
        EncoderMotor *tObjectPointer = sMotorControlListStart;
        // search last list element
        while (tObjectPointer->NextMotorControl != NULL)
        {
            tObjectPointer = tObjectPointer->NextMotorControl;
        }
        // insert current control in last element
        tObjectPointer->NextMotorControl = this;
    }
}
/*****************************************************
 * Static convenience functions affecting all motors.
 * If you have 2 motors, better use CarControl
 *****************************************************/

bool EncoderMotor::updateAllMotors()
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    // walk through list
    bool tMotorsNotStopped = false; // to check if motors are not stopped by aLoopCallback
    while (tEncoderMotorControlPointer != NULL)
    {
        tMotorsNotStopped |= tEncoderMotorControlPointer->updateMotor();
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }
    return tMotorsNotStopped;
}

/*
 * Waits until distance is reached
 */
void EncoderMotor::startRampUpAndWaitForDriveSpeedPWMForAll(uint8_t aRequestedDirection, void (*aLoopCallback)(void))
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    // walk through list
    while (tEncoderMotorControlPointer != NULL)
    {
        tEncoderMotorControlPointer->startRampUp(aRequestedDirection);
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }
    bool tMotorsNotStopped; // to check if motors are not stopped by aLoopCallback
    do
    {
        tMotorsNotStopped = EncoderMotor::updateAllMotors();
        if (aLoopCallback != NULL)
        {
            aLoopCallback(); // this may stop motors
        }
    } while (tMotorsNotStopped && !EncoderMotor::allMotorsStarted());
}

void EncoderMotor::startGoDistanceMillimeterForAll(int aRequestedDistanceMillimeter)
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    // walk through list
    while (tEncoderMotorControlPointer != NULL)
    {
        tEncoderMotorControlPointer->startGoDistanceMillimeter(aRequestedDistanceMillimeter);
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }
}

/*
 * Waits until distance is reached
 */
void EncoderMotor::goDistanceMillimeterForAll(int aRequestedDistanceMillimeter, void (*aLoopCallback)(void))
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    // walk through list
    while (tEncoderMotorControlPointer != NULL)
    {
        tEncoderMotorControlPointer->startGoDistanceMillimeter(aRequestedDistanceMillimeter);
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }
    waitUntilAllMotorsStopped(aLoopCallback);
}

bool EncoderMotor::allMotorsStarted()
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    bool tAllAreStarted = true;
    // walk through list
    while (tEncoderMotorControlPointer != NULL)
    {
        if (tEncoderMotorControlPointer->MotorRampState != MOTOR_STATE_DRIVE)
        {
            tAllAreStarted = false;
        }
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }
    return tAllAreStarted;
}

bool EncoderMotor::allMotorsStopped()
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    bool tAllAreStopped = true;
    // walk through list
    while (tEncoderMotorControlPointer != NULL)
    {
        if (tEncoderMotorControlPointer->MotorRampState != MOTOR_STATE_STOPPED)
        {
            tAllAreStopped = false;
        }
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }
    return tAllAreStopped;
}

/*
 * Start ramp down and busy wait for stop
 */
void EncoderMotor::stopAllMotorsAndWaitUntilStopped()
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    // walk through list
    while (tEncoderMotorControlPointer != NULL)
    {
        tEncoderMotorControlPointer->startRampDown();
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }

    /*
     * busy wait for stop
     */
    while (!allMotorsStopped())
    {
        updateAllMotors();
    }
}

void EncoderMotor::waitUntilAllMotorsStopped(void (*aLoopCallback)(void))
{
    do
    {
        EncoderMotor::updateAllMotors();
        if (aLoopCallback != NULL)
        {
            aLoopCallback();
        }
    } while (!EncoderMotor::allMotorsStopped());
}

void EncoderMotor::stopAllMotors(uint8_t aStopMode)
{
    EncoderMotor *tEncoderMotorControlPointer = sMotorControlListStart;
    // walk through list
    while (tEncoderMotorControlPointer != NULL)
    {
        tEncoderMotorControlPointer->stop(aStopMode);
        tEncoderMotorControlPointer = tEncoderMotorControlPointer->NextMotorControl;
    }
}
#endif // #ifdef ENABLE_MOTOR_LIST_FUNCTIONS
#endif // #if defined(USE_ENCODER_MOTOR_CONTROL)
#endif // #ifndef ENCODER_MOTOR_CONTROL_HPP
#pragma once
