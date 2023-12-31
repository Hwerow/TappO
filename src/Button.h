// Button.h

#ifndef BUTTON_H
#define BUTTON_H

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// calibration values
extern float xCalM; 
extern float yCalM; // gradients
extern float xCalC; 
extern float yCalC; // y axis crossing points
 

int8_t blockWidth = 20; // block size
int8_t blockHeight = 20;
int16_t blockX = 0, blockY = 0; // block position (pixels)

class ScreenPoint {
public:
int16_t x;
int16_t y;
 
ScreenPoint(){
// default contructor
}
 
ScreenPoint(int16_t xIn, int16_t yIn){
x = xIn;
y = yIn;
}
};

class Button {
private:
  int x;
  int y;
  int width;
  int height;
  const char* text;
  bool isSelected;
  bool isPressed;
  
public:
  // Method to set the button's pressed state
  void setPressed(bool state) {
    isPressed = state;
  }
    
  Button(int xPos, int yPos, int butWidth, int butHeight, const char* butText) {
    x = xPos;
    y = yPos;
    width = butWidth;
    height = butHeight;
    text = butText;
    isSelected = false; // Initially not selected
    isPressed = false; // Initially not pressed
   }
  // draws the buttons
  void render(TFT_eSPI &tft) { 
    if (isPressed) {
      tft.drawRoundRect(x, y, width, height, 4, TFT_YELLOW); // Yellow outline for selected button
      tft.setTextColor(TFT_YELLOW);
    } else {
      tft.drawRoundRect(x, y, width, height, 4, TFT_LIGHTGREY); // Grey outline for not selected button
      tft.setTextColor(TFT_SILVER);
    }

    tft.setCursor(x + 5, y + 5);
    tft.setTextSize(2);
    tft.print(text);
     
  }
  
   

  // Render the button based on the retrieved setting
  void render(bool settingValue) {
    
    isPressed = settingValue; // Update isPressed based on the retrieved setting 

    if (isPressed) {
      tft.drawRoundRect(x, y, width, height, 4, TFT_YELLOW);
      tft.setTextColor(TFT_YELLOW);
      } else {
      tft.drawRoundRect(x, y, width, height, 4, TFT_LIGHTGREY);
      tft.setTextColor(TFT_SILVER);
      }
    tft.setCursor(x + 5, y + 5);
    tft.setTextSize(2);
    tft.print(text); 
     
  }

  
  bool isClicked(ScreenPoint sp) {
    // return (sp.x >= x && sp.x <= (x + width) && sp.y >= y && sp.y <= (y + height));
  if ((sp.x >= x) && (sp.x <= (x + width)) && (sp.y >= y) && (sp.y <= (y + height))){
  return true;
  }
  else {
  return false;
  }
  }
   
};
 
ScreenPoint getScreenCoords(int16_t x, int16_t y){
int16_t xCoord = round((x * xCalM) + xCalC);
int16_t yCoord = round((y * yCalM) + yCalC);
if(xCoord < 0) xCoord = 0;
if(xCoord >= tft.width()) xCoord = tft.width() - 1;
if(yCoord < 0) yCoord = 0;
if(yCoord >= tft.height()) yCoord = tft.height() - 1;
return(ScreenPoint(xCoord, yCoord));
}

#endif  // BUTTON_H