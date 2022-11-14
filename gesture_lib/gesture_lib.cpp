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

#include "gesture_lib.h"

void gesture_lib::processGesture(const float window_filter_alpha, GestureType Gtype) {
    if (window_filter_alpha > 0.0) {
        noiseWindow3Filter(window_filter_alpha);
    }
    if (Gtype == GEST_DYNAMIC) {
        runDynamicGesture();
    }
    else if (Gtype == GEST_TRACKING) {

    }
}

void gesture_lib::resetGesture() {
    _reset_flag = true;
}


void gesture_lib::noiseWindow3Filter(const float alpha) {
    if (_reset_flag) {
        for(uint16_t i=0; i<_PixelArraySize; i++) {
            _nwin[0][i] = pixels[i]; // clear the filter
            _nwin[1][i] = pixels[i]; // clear the filter
            _nwin[2][i] = pixels[i]; // clear the filter
        }
    }
    else {
        // reset min and max values
        MaxPixelValue = -99999;
        for (uint16_t i=0; i<_PixelArraySize; i++) {
            _nwin[0][i] = _nwin[1][i];
            _nwin[1][i] = _nwin[2][i];
            _nwin[2][i] = pixels[i];
            pixels[i] = alpha * _nwin[1][i] + (1-alpha)*(_nwin[0][i] + _nwin[2][i])/2;
            if (pixels[i] > MaxPixelValue) MaxPixelValue = pixels[i];
        }
    }
}

void gesture_lib::runDynamicGesture() {
    memset(&dynamicResult, 0, sizeof(DynamicGestureResult));
    GestureEvent gest_event = GEST_NONE;

    // Static background subtraction
    {
        if (_reset_flag) {
            _state = STATE_INACTIVE;
            for(uint16_t i=0; i<_PixelArraySize; i++) {
                _foreground_pixels[i] = pixels[i]; // clear the filter
                _background_pixels[i] = pixels[i]; // clear the filter
            }
        }

        float background_alpha = BACKGROUND_FILTER_ALPHA;

        subtractBackground(LOW_PASS_FILTER_ALPHA, background_alpha);
    }

    if (_reset_flag) _reset_flag = false;

}

// Implement background subtraction by subtracting long exponential smoothing average from a shorter one
// (or simply the current pixel if alpha_short_avg is set to 1.0)
// alpha_long_avg should be smaller than alpha_short_avg
// Caller must keep static shart_avg_pixels[] and long_avg_pixels[]
// The bigger alpha long is, the more aggressive the high pass filter.
void gesture_lib::subtractBackground(const float alpha_short_avg, const float alpha_long_avg) {
    // reset min and max values
    MaxPixelValue = -99999;
    for (uint16_t i=0; i< _PixelArraySize; i++) {
        _background_pixels[i] = (1.0f - alpha_long_avg) * _background_pixels[i] + alpha_long_avg * (float)pixels[i];
        _foreground_pixels[i] = (1.0f - alpha_short_avg) * _foreground_pixels[i] + alpha_short_avg * (float)pixels[i];
        pixels[i] = (int16_t)(_foreground_pixels[i] - _background_pixels[i]);
        if (pixels[i] > MaxPixelValue) MaxPixelValue = pixels[i];
    }
}

void gesture_lib::interpn(const int w, const int h, const int interpolation_factor)
{
  int w2 = (w - 1) * interpolation_factor + 1;
  int h2 = (h - 1) * interpolation_factor + 1;
  int A, B, C, x, y;
  float x_ratio = 1.0f / (float)interpolation_factor;
  float y_ratio = 1.0f / (float)interpolation_factor;

  // First stretch in x-direction, index through each pixel of destination array. Skip rows in destination array
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w2; j++) {
      x = (int)(x_ratio * j);  // x index of original frame
      int index = i * w + x;  // pixel index of original frame
      if (x == w - 1) // last pixel on right edge of original frame
        _interp_pixels[i * w2 * interpolation_factor + j] = pixels[index]; // skip rows in dest array
      else {
        A = pixels[index];
        B = pixels[index + 1];
        float x_diff = (x_ratio * j) - x; // For 2x interpolation, will be 0, 1/2, 0, 1/2...
        _interp_pixels[i * w2 * interpolation_factor + j] = (int)(A + (B - A) * x_diff); // skip rows in dest array
      }
    }
  }
  // Then stretch in y-direction, index through each pixel of destination array
  for (int i = 0; i < h2; i++) {
    for (int j = 0; j < w2; j++) {
      y = (int)(y_ratio * i);  // y index of original frame
      int index = y * w2 * interpolation_factor + j;  // pixel index of frame
      if (y == h - 1) //  pixel on bottom of original frame
        _interp_pixels[i * w2 + j] = _interp_pixels[index];
      else {
        A = _interp_pixels[index];
        C = _interp_pixels[index + w2 * interpolation_factor];
        float y_diff = (y_ratio * i) - y;
        _interp_pixels[i * w2 + j] = (int16_t)(A + (C - A) * y_diff);
      }
    }
  }
}

// Zero out interpolated pixels below threshold value. Returns the number of pixels above the threshold
unsigned int gesture_lib::zeroPixelsBelowThreshold(const int threshold) {
    int pixelsAboveThresholdCount = _NUM_INTERP_PIXELS;
    for (unsigned int i = 0; i < _NUM_INTERP_PIXELS; i++) {
        if (_interp_pixels[i] < threshold) {
            _interp_pixels[i] = 0;
            pixelsAboveThresholdCount--;
        }
    }
    return pixelsAboveThresholdCount;
}

void gesture_lib::calcCenterOfMass(float *cmx, float *cmy, int *totalmass)
{
    int cmx_number=0, cmy_number=0;
    for (unsigned int i = 1; i < (_NUM_INTERP_COLS*_NUM_INTERP_ROWS)+1; i++) {
        cmx_number += (i%_NUM_INTERP_COLS)*_interp_pixels[i-1];
        cmy_number += (i/_NUM_INTERP_COLS)*_interp_pixels[i-1];
        *totalmass += _interp_pixels[i-1];
    }
    if (*totalmass == 0) {
        *totalmass = 1; // avoid NaN
    }
    *cmx = (float)cmx_number/(float)(*totalmass);
    *cmy = (float)cmy_number/(float)(*totalmass);
}
