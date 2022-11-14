/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Application: Gesture Library test program BETA
 * Author: C Gerrish @November 2022
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
*/

#include "mbed.h"
#include "USBSerial.h"

// Uncomment one device option for library to know which sensor is attached
//#define MAX25205_DEVICE
#define MAX25405_DEVICE

#include "MAX25x05.h"
#include "gesture_lib.h"

#define USE_SPI 1

#if USE_SPI

    #include <MAX25x05_SPI.h>

    SPI MAXspi_1(P5_1, P5_2, P5_0); // mosi, miso, sclk
    // The default settings of the SPI interface are 1MHz, 8-bit, Mode 0.
    // The max SPI frequency for MAX25405 is 6MHz - in the library 2MHz (2e6) will be used
    MAX25x05_SPI MAXIObus_1(MAXspi_1, 2e6, P5_5);

    DigitalOut selPin(P3_2, 0);        // SEL pin is set low to indicate to MAX25x05 that it's to use SPI mode

#else

    #include <MAX25x05_I2C.h>

    I2C MAXi2c_1(P3_4, P3_5);     // sda, scl   
    // 
    // The max I2C frequency for MAX25405 is 400k - in the library 100k (100e3) will be used
    MAX25x05_I2C MAXIObus_1(MAXi2c_1, 100e3, P5_5, 0);

    DigitalOut selPin(P3_2, 1);        // SEL pin is set high to indicate to MAX25x05 that it's to use I2C mode

#endif

// Bytes prefixed to pixel data to create data frame
const int NUM_INFO_BYTES = 40;


int main()
{

    //setup USB Serial comms for configuration option
    USBSerial serial(true, 0x0b6a, 0x4360, 0x0001);
    serial.set_blocking (true);

    MAX25x05 max25x_1(MAXIObus_1, P5_3);            // Interrupt pin for sensor 1
    //MAX25x05 max25x_2(MAXIObus_2, P3_3);            // Interrupt pin for sensor 2

    // Use the gesture library to manipulate/prepare pixels for output
    // ------------------------------------------------------------------
    gesture_lib gesture_1(SENSOR_COLS, SENSOR_ROWS);
    //int16_t pixels[NUM_SENSOR_PIXELS] = {'\0'};

    // Note if using 2 gesture sensors then the LED timings need to change [TODO]
    max25x_1.set_default_register_settings();           // Define for sensor number 1
    //max25x_2.set_default_register_settings();         // Define for sensor number 2

    max25x_1.enable_read_sensor_frames();
    //max25x_2.enable_read_sensor_frames();

    serial.printf("\r\nMAX25405 Pixel Extraction and Prep Test: version 1\r\n");

    while (true) {
        // If using INTB interrupt, the sensorDataReadyFlag will be set when the end-of-conversion occurs
        if (max25x_1.sensorDataReadyFlag) {
            // Check the interrupt pin to see if PWRON was set [[TODO]]
            uint8_t newIntVal = 0;

            max25x_1.getSensorPixelInts(gesture_1.pixels, false);
            gesture_1.processGesture(WINDOW_FILTER_ALPHA, gesture_1.GEST_DYNAMIC);
            
            for (uint8_t i = 0; i < NUM_SENSOR_PIXELS; i++) {
                serial.printf("%d,", gesture_1.pixels[i]);
            }
            serial.printf("%d%d\r\n",0xFFFF,0xFFFF);            // Adding placeholder for data checksum
            
            memset(gesture_1.pixels, '\0', NUM_SENSOR_PIXELS);
            
            max25x_1.sensorDataReadyFlag = false;
        }
    }
}
