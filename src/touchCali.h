// touchCali.h

#ifndef TOUCHCALI_H
#define TOUCHCALI_H

// Touch Screen pins cyd2usb touch uses HSPI pins not VSPI
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Include only  necessary headers
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include "Button.h"

/* float xCalM = 0.0; 
float xCalC = 0.0; 
float yCalM = 0.0; 
float yCalC = 0.0;  */

Preferences preferences;
bool isCalibrated;

// Most of the calibration code is from https://bytesnbits.co.uk/arduino-touchscreen-calibration-coding/
 
// TFT_eSPI tft = TFT_eSPI();

SPIClass mySpi = SPIClass(VSPI);  // this was HSPI which was wrong
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

void calibrateTouchScreen(){
TS_Point p;
int16_t x1,y1,x2,y2;
 
tft.fillScreen(TFT_BLACK);
// wait for no touch
while(ts.touched());
tft.drawFastHLine(10,20,20,TFT_RED);
tft.drawFastVLine(20,10,20,TFT_RED);
while(!ts.touched());
delay(50);
p = ts.getPoint();
x1 = p.x;
y1 = p.y;
tft.drawFastHLine(10,20,20,TFT_BLACK);
tft.drawFastVLine(20,10,20,TFT_BLACK);
delay(500);
while(ts.touched());
tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,TFT_RED);
tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,TFT_RED);
while(!ts.touched());
delay(50);
p = ts.getPoint();
x2 = p.x;
y2 = p.y;
tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,TFT_BLACK);
tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,TFT_BLACK);
 
int16_t xDist = tft.width() - 40;
int16_t yDist = tft.height() - 40;
 
// translate in form pos = m x val + c
// x
xCalM = (float)xDist / (float)(x2 - x1);
xCalC = 20.0 - ((float)x1 * xCalM);
// y
yCalM = (float)yDist / (float)(y2 - y1);
yCalC = 20.0 - ((float)y1 * yCalM);
 
Serial.print("x1 = ");Serial.print(x1);
Serial.print(", y1 = ");Serial.print(y1);
Serial.print("x2 = ");Serial.print(x2);
Serial.print(", y2 = ");Serial.println(y2);
Serial.print("xCalM = ");Serial.print(xCalM);
Serial.print(", xCalC = ");Serial.print(xCalC);
Serial.print(", yCalM = ");Serial.print(yCalM);
Serial.print(", yCalC = ");Serial.println(yCalC);
 
// Set isCalibrated to true once calibration is done
isCalibrated = true;
Serial.print("End of calibrateTouchScreen Is Calibrated ?  "); Serial.println(isCalibrated);

}
bool checkCalibrationInPreferences() {
  // Check if the preferences contain valid calibration data and isCalibrated flag
  return (preferences.getFloat("xCalM", 0.0) != 0.0 &&
          preferences.getFloat("xCalC", 0.0) != 0.0 &&
          preferences.getFloat("yCalM", 0.0) != 0.0 &&
          preferences.getFloat("yCalC", 0.0) != 0.0 &&
          preferences.getBool("isCalibrated", false));
}

void saveCalibrationToPreferences() {
  // Save calibration data and isCalibrated flag to Preferences
  preferences.begin("calibration", false); // Start preferences
  preferences.putFloat("xCalM", xCalM);
  preferences.putFloat("xCalC", xCalC);
  preferences.putFloat("yCalM", yCalM);
  preferences.putFloat("yCalC", yCalC);
  preferences.putBool("isCalibrated", isCalibrated);
  preferences.end(); // Call end() to save changes
}

void printCalibrationData() {
  preferences.begin("calibration", false); // Start preferences

  float xCalM = preferences.getFloat("xCalM", 0.0);
  float xCalC = preferences.getFloat("xCalC", 0.0);
  float yCalM = preferences.getFloat("yCalM", 0.0);
  float yCalC = preferences.getFloat("yCalC", 0.0);
  bool isCalibrated = preferences.getBool("isCalibrated", false);

  Serial.println("Calibration Data:");
  Serial.print("xCalM: "); Serial.println(xCalM);
  Serial.print("xCalC: "); Serial.println(xCalC);
  Serial.print("yCalM: "); Serial.println(yCalM);
  Serial.print("yCalC: "); Serial.println(yCalC);
  Serial.print("isCalibrated: "); Serial.println(isCalibrated);

  preferences.end(); // End preferences
}
 
 void useCalibrationData(){
 preferences.begin("calibration", false); // Start preferences
 bool isCalibrated = preferences.getBool("isCalibrated", false);
 if (isCalibrated){
  xCalM = preferences.getFloat("xCalM", 0.0);
  xCalC = preferences.getFloat("xCalC", 0.0);
  yCalM = preferences.getFloat("yCalM", 0.0);
  yCalC = preferences.getFloat("yCalC", 0.0);
 }

  preferences.end(); // End preferences
  }

#endif // TOUCHCALI_H