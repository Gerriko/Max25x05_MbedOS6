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

* Amendment: Convert library into c++ classes
* Author: C Gerrish @October 2022
*******************************************************************************
*/

#include "MAX25x05.h"

// STATUS
#define MAX25_INT_STATUS                        (0x00u)
// CONFIGURATION
#define MAX25_MAIN_CONFIG1                      (0x01u)
#define MAX25_MAIN_CONFIG2                      (0x02u)
#define MAX25_SEQ_CONGIG1                       (0x03u)
#define MAX25_SEQ_CONFIG2                       (0x04u)
#define MAX25_AFE_CONGIG                        (0x05u)
#define MAX25_LED_CONFIG                        (0x06u)
// ADC START ADDRESS FOR 60 LED'S (HIGH/LOW ORDER)
#define MAX25_ADC_START_H                       (0x10u)
// CHANNEL GAIN TRIMS
#define MAX25_COL_GAIN_2                        (0xA5u)
#define MAX25_COL_GAIN_4                        (0xA6u)
#define MAX25_COL_GAIN_6                        (0xA7u)
#define MAX25_COL_GAIN_8                        (0xA8u)
#define MAX25_COL_GAIN_10                       (0xA9u)
// LED CONTROL
#define MAX25_LED_CTRL                          (0xC1u)


/*
* This is the interrupt handler for sensor1 to handle end-of-conversion interrupts on the INTB1 pin
*/
void MAX25x05::intb_handler()
{
    sensorDataReadyFlag = true;
}


void MAX25x05::begin(int hz) {

    INTERFACE_FUNC(begin)(hz);

}

void MAX25x05::set_default_register_settings() {
    
    uint32_t ledDrvLevel = 0x0F;                                    // 0b1111 max duty cycle 16/16 (200mA)

    INTERFACE_FUNC(reg_write)(MAX25_MAIN_CONFIG1, 0x04);            // Set EOCINTE to 1: Enables the end-of-conversion interrupt.
    INTERFACE_FUNC(reg_write)(MAX25_MAIN_CONFIG2, 0x02);            // Not sure why this is set to 0x02
#if defined(MAX25405_DEVICE)
    INTERFACE_FUNC(reg_write)(MAX25_SEQ_CONGIG1, 0x84);             // Changed to 0x84 (was 0x24) SDLY=2 (sample-delay times), TIM=2 (integration time for the ADC conversion)
    INTERFACE_FUNC(reg_write)(MAX25_SEQ_CONFIG2, 0x8C);             // NRPT=4 (Number of Repeats), NCDS=3 (Number of Coherent Double Samples)
#elif defined(MAX25205_DEVICE)
    INTERFACE_FUNC(reg_write)(MAX25_SEQ_CONGIG1, 0x04); // SDLY=0, TIM=2
    INTERFACE_FUNC(reg_write)(MAX25_SEQ_CONFIG2, 0xAC); // NRPT=5, NCDS=3
    ledDrvLevel = 0x0A;
#endif

    INTERFACE_FUNC(reg_write)(MAX25_AFE_CONGIG, 0x08);              // Coarse Ambient Light Compensation Enabled

    INTERFACE_FUNC(reg_write)(MAX25_LED_CONFIG, ledDrvLevel);       // LED power

    // There are ten different 4-bit column gains for the entire 60-channel array. 
    // Each trim value applies to one of the ten columns in the pixel array.
    INTERFACE_FUNC(reg_write)(MAX25_COL_GAIN_2, 0x88);              // 0b1000 for CGAIN2 and 0b1000 for CGAIN1 (gain of 1.00)
    INTERFACE_FUNC(reg_write)(MAX25_COL_GAIN_4, 0x88);              // 0b1000 for CGAIN4 and 0b1000 for CGAIN3 (gain of 1.00)
    INTERFACE_FUNC(reg_write)(MAX25_COL_GAIN_6, 0x88);              // 0b1000 for CGAIN6 and 0b1000 for CGAIN5 (gain of 1.00)
    INTERFACE_FUNC(reg_write)(MAX25_COL_GAIN_8, 0x88);              // 0b1000 for CGAIN8 and 0b1000 for CGAIN7 (gain of 1.00)
    INTERFACE_FUNC(reg_write)(MAX25_COL_GAIN_10, 0x88);             // 0b1000 for CGAIN10 and 0b1000 for CGAIN9 (gain of 1.00)

    INTERFACE_FUNC(reg_write)(MAX25_LED_CTRL, 0x0A);                // GAINSEL:1 (Internal trim val), DRV_EN:0 (disabled), ELED_EN:1(enabled), ELED_POL:0 (nMOS)
    
}

/*
* This function starts the monitoring of the INTB interrupt
*/
void MAX25x05::enable_read_sensor_frames(void) {

    _intb.fall(callback(this, &MAX25x05::intb_handler)); // Add INTB interrupt handler

    // Read status reg to clear interrupt
    uint8_t status_reg;
    INTERFACE_FUNC(reg_read)(MAX25_INT_STATUS, 1, &status_reg);

    read_sensor_frames_enabled = true;

}

/*
* This function stops the monitoring of the INTB interrupt or polling
*/
void MAX25x05::disable_read_sensor_frames() {
    _intb.fall(0); // Remove interrupt handler

    read_sensor_frames_enabled = false;

}

void MAX25x05::getInterruptStatus(uint8_t &IntValue) {
    INTERFACE_FUNC(reg_read)(MAX25_INT_STATUS, 1, &IntValue);
}

void MAX25x05::getSensorPixelInts(int16_t pixels[], const bool flip_sensor_pixels) {
  unsigned char reg_vals[NUM_SENSOR_PIXELS*2];
  INTERFACE_FUNC(reg_read)(MAX25_ADC_START_H, NUM_SENSOR_PIXELS*2, reg_vals);

  for (int i = 0; i < NUM_SENSOR_PIXELS; i++) {
    pixels[i] = convertTwoUnsignedBytesToInt(reg_vals[2 * i], reg_vals[2 * i + 1]);
  }

  if (flip_sensor_pixels) {
    for (int i = 0; i < NUM_SENSOR_PIXELS/2; i++) {
      int temp = pixels[i];
      pixels[i] = pixels[NUM_SENSOR_PIXELS-1-i];
      pixels[NUM_SENSOR_PIXELS-1-i] = temp;
    }
  }
}

int16_t MAX25x05::convertTwoUnsignedBytesToInt(uint8_t hi_byte, uint8_t lo_byte)
{
    uint16_t intval = (uint16_t)(hi_byte << 8 | lo_byte);

    if (intval & 0x8000)
        return -(int16_t)(~intval) - 1;
    else
        return (int16_t)intval;

    // Convert unsigned to 2's complement
    //if ((1 << 15) < intval) intval -= (1 << 16);
    //return intval;
}
