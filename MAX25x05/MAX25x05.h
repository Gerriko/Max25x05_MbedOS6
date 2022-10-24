/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All rights Reserved.
*
* This software is protected by copyright laws of the United States and
* of foreign countries. This material may also be protected by patent laws
* and technology transfer regulations of the United States and of foreign
* countries. This software is furnished under a license agreement and/or a
* nondisclosure agreement and may only be used or reproduced in accordance
* with the terms of those agreements. Dissemination of this information to
* any party or parties not specified in the license agreement and/or
* nondisclosure agreement is expressly prohibited.
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*
*******************************************************************************
* Amendment: Convert Maxim Firmware Framework library into c++ classes for MbedOS6.16
* Author: C Gerrish @October 2022
*******************************************************************************
*/

#ifndef __MAX25X05_H__
#define __MAX25X05_H__

#include "mbed.h"

// Sensor constants. Declared as constants so arrays can be statically sized
#define SENSOR_COLS                             (10u)
#define SENSOR_ROWS                             (6u)
#define NUM_SENSOR_PIXELS                       (SENSOR_COLS * SENSOR_ROWS)


/*
* MAX25x05 Classes
*
*/


#define INTERFACE_FUNC(func)   (_BusInterface->func)


class MAX25x05_BusInterface
{
public:
    virtual void begin(int hz) = 0;

    virtual int reg_write(const uint8_t reg_addr, const uint8_t reg_val) = 0;

    virtual int reg_read(const uint8_t reg_addr, const uint8_t num_bytes, uint8_t reg_vals[]) = 0;

};


class MAX25x05
{

public:
    MAX25x05(MAX25x05_BusInterface &interface, PinName intbpin, PinName rLEDpin = LED1, PinName gLEDpin = LED2):
        _BusInterface(&interface), _intb(intbpin), _rLED(rLEDpin), _gLED(gLEDpin)
        {
        };

    ~MAX25x05();

    void begin(int hz);

    void set_default_register_settings(void);

    void enable_read_sensor_frames(void);

    void disable_read_sensor_frames(void);

    void getSensorPixels(int pixels[], const bool flip_sensor_pixels);

    // Data ready flags
    volatile bool sensorDataReadyFlag = false; // Data ready flag, set by the end-of-conversion interrupt

private:

    void intb_handler(void);
    int convertTwoUnsignedBytesToInt(uint8_t hi_byte, uint8_t lo_byte);

    MAX25x05_BusInterface *_BusInterface;

    DigitalOut _rLED;
    DigitalOut _gLED;

    InterruptIn _intb;

    bool read_sensor_frames_enabled = false;

};


#endif // __MAX25X05_H__
