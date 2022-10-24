/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "USBSerial.h"

// Uncomment one device option for library to know which sensor is attached
//#define MAX25205_DEVICE
#define MAX25405_DEVICE

#include "MAX25x05.h"

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


int main()
{

    //setup USB Serial comms for configuration option
    USBSerial serial(true, 0x0b6a, 0x4360, 0x0001);

    MAX25x05 gesture1(MAXIObus_1, P5_3);            // Interrupt pin for sensor 1
    //MAX25x05 gesture2(MAXIObus_2, P3_3);            // Interrupt pin for sensor 2

    static int pixels[NUM_SENSOR_PIXELS];

    // Note if using 2 gesture sensors then the LED timings need to change [TODO]
    gesture1.set_default_register_settings();       // Define for sensor number 1
    //gesture2.set_default_register_settings();       // Define for sensor number 2

    gesture1.enable_read_sensor_frames();
    //gesture2.enable_read_sensor_frames();

    serial.printf("\r\nMAX25405 Comms Test\r\n");

    while (true) {
        // If using INTB interrupt, the sensorDataReadyFlag will be set when the end-of-conversion occurs
        if (gesture1.sensorDataReadyFlag) {
            gesture1.getSensorPixels(pixels, false);
            
            for (uint8_t i = 0; i < NUM_SENSOR_PIXELS; i++) {
                if (i < (NUM_SENSOR_PIXELS-1)) serial.printf("%u,", pixels[i]);
                else  serial.printf("%u\r\n", pixels[i]);
            }
            gesture1.sensorDataReadyFlag = false;
        }

    }
}
