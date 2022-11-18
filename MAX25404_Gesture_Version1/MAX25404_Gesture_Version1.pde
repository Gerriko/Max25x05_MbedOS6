/**
 * Read Serial Output from MbedOS MAX25405 Gesture Sensor firmware on MAX32620FTHR
 * MAX32620FTHR applies foreground and background subtraction
 * Gesture sensing algorithm replicating Gesture Sensor Firmware Framework version 1.1
 *
 * Reads data from the serial port and produces graphical output on screen of each pixel value.
 * Then applies smoothing and filtering of data and displays this output too to highlight differences
 *
 * Version 2.1: Added in L/R/U/D Gesture detection in Progress monitoring (very basic and not accurate at present)
 *
 * Written by C. Gerrish 1 November 2022
 *

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.

**/

import processing.serial.*;

Serial myPort;  // Create object from Serial class
final int lf = 10;    // Linefeed in ASCII
final int cr = 13;    // Carriage Return in ASCII

final static int SENSOR_XRES = 10;
final static int SENSOR_YRES = 6;
final static int NUMBER_SENSOR_PIXELS = 60;

// Interpolation used in gesture algorithm
final static int INTERP_FACTOR = 4;
final static int INTERP_XRES = ((SENSOR_XRES-1)*INTERP_FACTOR+1);
final static int INTERP_YRES = ((SENSOR_YRES-1)*INTERP_FACTOR+1);
final static int NUM_INTERP_PIXELS = (INTERP_XRES*INTERP_YRES);

final static int ZERO_CLAMP_THRESHOLD_FACTOR = 6;
final static int ZERO_CLAMP_THRESHOLD = 12;
final static int END_DETECTION_THRESHOLD = 250; /*Changed from 250 for 400um device*/

//final static float DY_PIXEL_SCALE = 1.42857; /*10.0f/7.0f*/
final static float DY_PIXEL_SCALE = 1.66667; /*10.0f/6.0f*/

static int[][] pvar = new int[2][NUMBER_SENSOR_PIXELS];
static int startvar = 0;

static int[] interp_pixels = new int[NUM_INTERP_PIXELS];

static String myString = null;

static int GestureInProgress = 0;

static PixelProperty[] pixBoxes;

static int Save2File = 0;
Table table;

// Save delta CMx & CMy & CMlen and the delta of delta CMx & CMy
static int[] prev_cmx = {0,-1,-1,-1};
static int[] prev_cmy = {0,-1,-1};

static int[][] Delta_cmx = new int[2][2];
static int[][] Delta_cmy = new int[2][2];

static int[] pvar_sum = new int[12];
static int pvar_cntr = 0;

static int[][] pvar_click = new int[2][4];

void setup()
{
  size(1060, 700);

  // I know that the first port in the serial list on my mac
  // is always my  FTDI adaptor, so I open Serial.list()[0].
  // On Windows machines, this generally opens COM1.
  // Open whatever port is the one you're using.
  String portName = Serial.list()[0];
  try{
    myPort = new Serial(this, portName, 115200);
  } catch(Exception e) {
    println("Error Opening Serial Port");
    while (true) {;;;}
  }
  imageMode(CENTER);
  background(0);            // Set background to black

  pixBoxes = new PixelProperty[NUMBER_SENSOR_PIXELS];

  int index = 0;
  int x1 = 0;
  int x2 = 0;
  int x3 = 0;
  for (int y = 0; y < SENSOR_YRES; y++) {
    for (int x = 0; x < SENSOR_XRES; x++) {
      if (x < 3) pixBoxes[index++] = new PixelProperty(30, 730, 30, x, y, 1, x1++);
      else if (x < 7) {
        if (y == 2 || y == 3) pixBoxes[index++] = new PixelProperty(30, 730, 30, x, y, 2, (6*3)+x2++);
        else {
          if ((y == 1 || y == 4) && (x == 4 || x == 5)) pixBoxes[index++] = new PixelProperty(30, 730, 30, x, y, 2, (6*3)+x2++);
          else pixBoxes[index++] = new PixelProperty(30, 730, 30, x, y, 2, (6*3)+x2++);
        }
      }
      else pixBoxes[index++] = new PixelProperty(30, 730, 30, x, y, 3, (6*7)+x3++);
    }
  }

  table = new Table();

  table.addColumn("CMx");
  table.addColumn("CMy");
  table.addColumn("dxCMx");
  table.addColumn("dyCMy");

}

void draw()
{
  if (myPort != null) {
    while (myPort.available() > 0) {
      myString = myPort.readStringUntil(lf);
      if (myString != null) {
        // Remove last char which should be carriage return
        if (myString.length() > NUMBER_SENSOR_PIXELS) {
          // Note that I had placed an additional checksum in my serial output for debugging purposes - hence 12 chars are removed.
          // If using the Github files, please amend here as necessary.
          myString = myString.substring(0, myString.length()-12);
          // The last char should now be a comma
          int[] GesturePixels = int(split(myString, ','));

          if (GesturePixels.length == NUMBER_SENSOR_PIXELS+1) {

            if (GestureInProgress == 1) {
              if (Save2File == 1) background(0,90,0);        // Change background colour when gesture in motion to indicate data logging
              else background(0,90,100);                     // Change background colour when gesture in motion (no data logging)
            }
            else background(0);                              // Set background to black - no object detected

            int Filtered_maxPixel = -99999;
            int Filtered_minPixel = 99999;

            pvar_cntr = 0;

            for (int i=0; i< NUMBER_SENSOR_PIXELS; i++) {
              if (GesturePixels[i] > Filtered_maxPixel) Filtered_maxPixel = GesturePixels[i];
              if (GesturePixels[i] < Filtered_minPixel) Filtered_minPixel = GesturePixels[i];

              if (GestureInProgress == 1) {
                if (startvar == 1) {
                  pvar[1][i] = (int(map(GesturePixels[i], -32768, 32767, -256, 255)) - pvar[0][i]);
                  stroke(150);              // Now set a grey color for points
                  strokeWeight(2);
                  // Using "height" to control "width" measurement to align more to the left
                  line(height/2, i*10+50, height/2 + map(pvar[1][i], -256, 255, -height/2, height/2), i*10+50);
                  pixBoxes[i].displayVar(pvar[1][i]);
                  if (pixBoxes[i].getCentrePix(i) == 0) {
                    pvar_sum[pvar_cntr] = pvar[1][i];
                    pvar_cntr++;
                  }
                }
                pvar[0][i] = int(map(GesturePixels[i], -32768, 32767, -256, 255));
              }

              // For middle band of pixels use a different colour
              if (i < 20) {
                stroke(210, 52, 0);
                strokeWeight(4);
              }
              else if (i >= 20 && i < 40) {
                stroke(210, 210, 0);
                strokeWeight(5);
              }
              else {
                stroke(0, 210, 52);
                strokeWeight(4);
              }
              point(map(GesturePixels[i], -32768, 32767, 0, 600), i*10+50);
              pixBoxes[i].displayPoint(GesturePixels[i]);

              noStroke();
              pixBoxes[i].update(i, map(constrain(GesturePixels[i], -1275, 12750), -1275, 12750, 0, 255));
              pixBoxes[i].display();
              fill(255);
              textSize(16);
              text("HEAT MAP", 740, 25);
              text("GESTURE MAP", 740, 255);
              text("RESULTS", 740, 485);
              strokeWeight(1);
              stroke(255);
              noFill();
              rect(730,30,300,180);
              rect(730,height/2-90,300,180);
              fill(255);
              rect(730, height-210,300,180);

            }

            if (GestureInProgress == 1 && startvar == 0) startvar = 1;
            else if (GestureInProgress == 0 && startvar == 1) startvar = 0;

            interpn(GesturePixels, interp_pixels, SENSOR_XRES, SENSOR_YRES, INTERP_FACTOR);

            // Thresholding - zero out pixels below some percent of peak
            int threshold = Filtered_maxPixel / ZERO_CLAMP_THRESHOLD_FACTOR;
            for (int i = 0; i < NUM_INTERP_PIXELS; i++) {
              if (interp_pixels[i] < threshold) {
                interp_pixels[i] = 0;
              }
            }
            // and Thresholding again - zero out below fixed threshold
            threshold = ZERO_CLAMP_THRESHOLD;
            for (int i = 0; i < NUM_INTERP_PIXELS; i++) {
              if (interp_pixels[i] < threshold) {
                interp_pixels[i] = 0;
              }
            }
            // Center of mass
            // Final part determines if a gesture is in progress or not
            float[] cmx = {0.0};
            float[] cmy = {0.0};

            int[] totalmass = {0};
            if (Filtered_maxPixel >= END_DETECTION_THRESHOLD && (Filtered_maxPixel-Filtered_minPixel) > 1250) {
              calcCenterOfMass(interp_pixels, INTERP_XRES, INTERP_YRES, cmx, cmy, totalmass); // Only calculate COM if there is a pixel above the noise (avoid divide-by-zero)
              cmx[0] = cmx[0]/INTERP_FACTOR;
              cmy[0] = cmy[0]/INTERP_FACTOR * DY_PIXEL_SCALE; // scale y so it has same unit dimension as x

              // Calculate Delta's
              for (int x = 0; x < 2; x++) {
                Delta_cmx[x][0] = Delta_cmx[x][1];
                Delta_cmy[x][0] = Delta_cmy[x][1];
              }
              Delta_cmx[0][1] = int(cmx[0]*1000) - prev_cmx[0];
              Delta_cmy[0][1] = int(cmy[0]*1000) - prev_cmy[0];

              Delta_cmx[1][1] = Delta_cmx[0][1] - Delta_cmx[0][0];
              Delta_cmy[1][1] = Delta_cmy[0][1] - Delta_cmy[0][0];

              // -----------------------------------------------------------
              // GESTURE RECOGNITION
              // ===========================================================
              // Showing centre of mass in separate gesture box starting at
              // x = 730, length 300, y = height/2-90, height = 180

              float[] mappedCMx = {0.0, 0.0, 0.0, 0.0, 0.0};      //map(cmx[0], 0, 10, 0, 300);
              float[] mappedCMy = {0.0, 0.0, 0.0, 0.0};      //map(cmy[0], 0, 10, 0, 180);

              // because there's more x pixels than y pixels
              mappedCMx[4] = map((prev_cmx[3]/1000.0), 0, 10, 0, 300);

              if (prev_cmx[2] != -1) {
                stroke(105);
                strokeWeight(7);
                mappedCMx[3] = map((prev_cmx[2]/1000.0), 0, 10, 0, 300);
                mappedCMy[3] = map((prev_cmy[2]/1000.0), 0, 10, 0, 180);
                point(730 + mappedCMx[3], height/2-90 + mappedCMy[3]);
              }
              if (prev_cmx[1] != -1) {
                stroke(155);
                strokeWeight(8);
                mappedCMx[2] = map((prev_cmx[1]/1000.0), 0, 10, 0, 300);
                mappedCMy[2] = map((prev_cmy[1]/1000.0), 0, 10, 0, 180);
                point(730 + mappedCMx[2], height/2-90 + mappedCMy[2]);
              }
              if (prev_cmx[0] != 0) {
                stroke(205);
                strokeWeight(9);
                mappedCMx[1] = map((prev_cmx[0]/1000.0), 0, 10, 0, 300);
                mappedCMy[1] = map((prev_cmy[0]/1000.0), 0, 10, 0, 180);
                point(730 + mappedCMx[1], height/2-90 + mappedCMy[1]);
              }
              stroke(255);
              strokeWeight(10);
              mappedCMx[0] = map(cmx[0], 0, 10, 0, 300);
              mappedCMy[0] = map(cmy[0], 0, 10, 0, 180);
              point(730 + mappedCMx[0], height/2-90 + mappedCMy[0]);

              noFill();
              strokeWeight(1);
              rect(730,height/2-90,300,180);

              if (mappedCMx[0] < 120 && (mappedCMx[1] > 210 || mappedCMx[2] > 210 || mappedCMx[3] > 210 || mappedCMx[4] > 210)) {
                // Right to Left gesture
                fill(0,0,255);
                textSize(60);
                text("<-LEFT", 750, height-100);
              }
              else if (mappedCMx[0] > 210 && ((mappedCMx[1] < 120 && mappedCMx[1] > 0) || (mappedCMx[2] < 120 && mappedCMx[2] > 0) || (mappedCMx[3] < 120 && mappedCMx[3] > 0) || (mappedCMx[4] < 120 && mappedCMx[4] > 0))) {
                // Left to Right gesture
                fill(0,0,255);
                textSize(60);
                text("RIGHT->", 750, height-100);
              }
              else {
                if (mappedCMy[0] < 64 && (mappedCMy[1] > 116 || mappedCMy[2] > 116 || mappedCMy[3] > 116)) {
                  // Right to Left gesture
                  fill(0,0,255);
                  textSize(60);
                  text("UP", 750, height-100);
                }
                else if (mappedCMy[0] > 116 && (mappedCMy[1] < 72 || mappedCMy[2] < 72 || mappedCMy[3] < 72)) {
                  // Right to Left gesture
                  fill(0,0,255);
                  textSize(60);
                  text("DOWN", 750, height-100);
                }
              }

              //println("CMX:"+nf(cmx[0])+ " CMY:"+nf(cmy[0]));
              if (Save2File == 1) {
                TableRow newRow = table.addRow();
                newRow.setInt("CMx", int(cmx[0]*1000));
                newRow.setInt("CMy", int(cmy[0]*1000));
                newRow.setInt("dxCMx", Delta_cmx[0][1]);
                newRow.setInt("dyCMy", Delta_cmy[0][1]);
              }
              prev_cmx[3] = prev_cmx[2];
              prev_cmx[2] = prev_cmx[1];
              prev_cmy[2] = prev_cmy[1];
              prev_cmx[1] = prev_cmx[0];
              prev_cmy[1] = prev_cmy[0];
              prev_cmx[0] = int(cmx[0]*1000);
              prev_cmy[0] = int(cmy[0]*1000);

              GestureInProgress = 1;

            }
            else {
              if (GestureInProgress == 1) {
                if (table.getRowCount() > 1 && Save2File == 0) {
                  saveTable(table, "data/CofMass.csv");
                  table.clearRows();
                }
              }
              GestureInProgress = 0;
            }
          }
        }
      }
    }
  }
}

void interpn(final int[] pixels, int[] interp_pixels, final int w, final int h, final int interpolation_factor)
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
        interp_pixels[i * w2 * interpolation_factor + j] = pixels[index]; // skip rows in dest array
      else {
        A = pixels[index];
        B = pixels[index + 1];
        float x_diff = (x_ratio * j) - x; // For 2x interpolation, will be 0, 1/2, 0, 1/2...
        interp_pixels[i * w2 * interpolation_factor + j] = (int)(A + (B - A) * x_diff); // skip rows in dest array
      }
    }
  }
  // Then stretch in y-direction, index through each pixel of destination array
  for (int i = 0; i < h2; i++) {
    for (int j = 0; j < w2; j++) {
      y = (int)(y_ratio * i);  // y index of original frame
      int index = y * w2 * interpolation_factor + j;  // pixel index of frame
      if (y == h - 1) //  pixel on bottom of original frame
        interp_pixels[i * w2 + j] = interp_pixels[index];
      else {
        A = interp_pixels[index];
        C = interp_pixels[index + w2 * interpolation_factor];
        float y_diff = (y_ratio * i) - y;
        interp_pixels[i * w2 + j] = (int)(A + (C - A) * y_diff);
      }
    }
  }
}

void calcCenterOfMass(final int[] pixels, final int xres, final int yres, float[] cmx, float[] cmy, int[] totalmass)
{
  int cmx_numer=0, cmy_numer=0;
  for (int i = 0; i < xres*yres; i++) {
    cmx_numer += (i%xres)*pixels[i];
    cmy_numer += (i/xres)*pixels[i];
    totalmass[0] += pixels[i];
  }
  if (totalmass[0] == 0) {
    totalmass[0] = 1; // avoid NaN
  }
  cmx[0] = (float)cmx_numer/(float)(totalmass[0]);
  cmy[0] = (float)cmy_numer/(float)(totalmass[0]);
}

void mouseClicked() {
  if (Save2File == 0) Save2File = 1;
  else Save2File = 0;
}
