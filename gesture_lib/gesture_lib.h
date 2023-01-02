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
*******************************************************************************
* Amendment: Convert Maxim Firmware Framework library into c++ classes for MbedOS6.16
* Author: C Gerrish @October 2022
* Note this beta version is still work in progress
*******************************************************************************
*/

#ifndef __GESTURE_LIB_H__
#define __GESTURE_LIB_H__

#include "mbed.h"
#include <cstdint>

#define DY_PIXEL_SCALE              (1.66667) /*10.0f/6.0f*/

// Interpolation used in gesture algorithm
#define INTERP_FACTOR               4

#define BACKGROUND_FILTER_ALPHA     (0.10F) /*Changed from 0.10f for 400um device*/
#define LOW_PASS_FILTER_ALPHA       (1.0F)
#define ZERO_CLAMP_THRESHOLD_FACTOR (6u)
#define ZERO_CLAMP_THRESHOLD        (10u)
#define START_DETECTION_THRESHOLD   (400u) /*Changed from 400 for 400um device*/
#define END_DETECTION_THRESHOLD     (250u) /*Changed from 250 for 400um device*/
#define WINDOW_FILTER_ALPHA         (0.5F)

const uint8_t PIXELSECTOR[60] = {1,1,1,1,1,2,2,2,2,2,1,1,1,1,0,0,2,2,2,2,1,1,1,0,0,0,0,2,2,2,4,4,4,0,0,0,0,3,3,3,4,4,4,4,0,0,3,3,3,3,4,4,4,4,4,3,3,3,3,3};


  
class gesture_lib
{

public:

    gesture_lib(const uint8_t PixelArrayCols, const uint8_t PixelArrayRows):
        _PixelArrayCols(PixelArrayCols), _PixelArrayRows(PixelArrayRows), _PixelArraySize(PixelArrayCols*PixelArrayRows),
        _NUM_INTERP_COLS((_PixelArrayCols-1)*INTERP_FACTOR+1), _NUM_INTERP_ROWS((_PixelArrayRows-1)*INTERP_FACTOR+1),
        _NUM_INTERP_PIXELS(_NUM_INTERP_COLS*_NUM_INTERP_ROWS)
    {
        pixels = new int16_t[_PixelArraySize];
        _nwin[0] = new int16_t[_PixelArraySize];
        _nwin[1] = new int16_t[_PixelArraySize];
        _nwin[2] = new int16_t[_PixelArraySize];
        _interp_pixels = new int16_t[_NUM_INTERP_PIXELS];
        _foreground_pixels = new float[_PixelArraySize];
        _background_pixels = new float[_PixelArraySize];

    };
    
    ~gesture_lib()
    {
        delete []pixels;
        delete []_nwin[0];
        delete []_nwin[1];
        delete []_nwin[2];
        delete []_interp_pixels;
        delete []_foreground_pixels;
        delete []_background_pixels;
    };

    /* Enumerate for gesture type analysis */
    typedef enum {
        GEST_DYNAMIC,
        GEST_TRACKING
    } GestureType;

    // Gesture states
    typedef enum {STATE_INACTIVE, GESTURE_IN_PROGRESS} GestureState;

    // Structure to store dynamic gesture results
    typedef struct {
        uint8_t state;              // 0: inactive; 1: object detected; 2: rotation in progress
        uint32_t n_sample;          // The current sample number of this gesture
        int maxpixel;               // Maximum pixel value for this frame
        float cmx;                  // Object x-position
        float cmy;                  // Object y-position
        uint32_t CoM_Intensity;     // Object CoM intensity value
    } DynamicGestureResult;

    DynamicGestureResult dynamicResult;

    int16_t *pixels;
    int MaxPixelValue = -99999;


    void processGesture(const float window_filter_alpha, GestureType Gtype);
    void resetGesture(void);

private:
    void noiseWindow3Filter(const float alpha);
    void runDynamicGesture(void);
    void subtractBackground(const float alpha_short_avg, const float alpha_long_avg);
    void interpn();
    unsigned int zeroPixelsBelowThreshold(const int threshold);
    void calcCenterOfMass(float *cmx, float *cmy, int32_t *totalmass);

    const uint8_t _PixelArrayCols;
    const uint8_t _PixelArrayRows;
    const uint8_t _PixelArraySize;
    const uint8_t _NUM_INTERP_COLS;
    const uint8_t _NUM_INTERP_ROWS;
    const uint16_t _NUM_INTERP_PIXELS;

    // Enumerate gesture events here
    typedef enum {GEST_NONE, GEST_PLACEHOLDER} GestureEvent;

    GestureState _state =   STATE_INACTIVE;

    bool _reset_flag =      true;
    uint32_t _n_sample =    0;
    uint32_t _n_frame =     0;

    int16_t *_nwin[3];
    int16_t *_interp_pixels;
    float *_foreground_pixels;
    float *_background_pixels;

};


#endif  // __GESTURE_LIB_H__
