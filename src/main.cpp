/* 
cyd2usb Program for Touch
in setup
  first use CYD
launch FW title screen
  calibrate touch?
      yes 
launch calibrate
  do the calibration 
  save values in preferences   
  subsequent use 
use saved calibration details 

launch menu screen shows parameters currently set
Only selected
Change parameters?
    yes 
select by touching button set = highlighted in yellow; unset grey
once chosen touch OK to save and DONE to continue
save parameters in preferences 
use parameters all bool in config.h main code

then in setup
run the wm parameter settings input this seems the optimal way to get the Auth_B batch details and save them
Auth_B based on brewF true run the wm parameter settings to get the batch details

UTC_offset for clock. for Sydney Australia

save to wm preferences 
use wm data

v0.1 
Using TapList.h for getting data from BF and displaying on TFT screen

v0.2 
Testing
ABV updated

v 1.0
Initial Release


 */


#include <Arduino.h>
#include <SPI.h>
// #include <XPT2046_Touchscreen.h> in touchcali
// #include <TFT_eSPI.h> // in button.h
#include "Math.h"

// code split out hopefully for some clarity?
#include "Button.h" // Include the Button header with all the button logic and the tft
#include "config.h" // has #include "touchCali.h" 
#include "TapList.h"    // Download from BF and screen display

#include <WiFi.h>             //for ESP32
// #include <WiFiClientSecure.h> // for BF API in TapList.h
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "NotoSans_Bold.h" // 
#include "OpenFontRender.h"
#define TTF_FONT NotoSans_Bold // The font is referenced with the array name look in the file!!
// OpenFontRender ofr; 

#include <WiFiManager.h>
// #include <ArduinoJson.h>
// ------------------------------------------------------------------------------------

const char version[] = "1.0";

// ---------------------------------------------------------------------------------------------------------
// Debugger https://github.com/RalphBacon/224-Superior-Serial.print-statements/tree/main/Simple_Example
#define Debugger 1

#if Debugger == 1
#define Debug(x) Serial.print(x)
#define Debugln(x) Serial.println(x)
#define Debugf(x) Serial.printf(x) // throws errors not used
#else
#define Debug(x)
#define Debugln(x)
#define Debugf(x)
#endif

// cyd LED pins
#define CYD_LED_RED 4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE 17

// custom hostname  The valid letters for hostname are a-z 0-9 and -  works
const char *hostname = "TappOCYD";

// preferences used in preference to EEPROM which didn't work on ESP32 and instead of the wm config json file
#include <Preferences.h>
extern bool rstDONE;

/* this now in touchCali // Touch Screen pins cyd2usb touch uses HSPI pins not VSPI
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass mySpi = SPIClass(VSPI);  // this was HSPI which was wrong
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ); */

// TFT_eSPI tft = TFT_eSPI(); // is now in Button.h

// chatGPT calibrate only once and save to Preferences. Check if values exist during setup if not calibrated 
// i.e. click on red cross hairs
// float xCalM = 0.0, xCalC = 0.0, yCalM = 0.0, yCalC = 0.0;

// these are used in touchCali and Button.h  set to extern
float xCalM = 0.0; 
float xCalC = 0.0; 
float yCalM = 0.0; 
float yCalC = 0.0;

// touch screen debounce
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100; // Debounce time in milliseconds
bool buttonState = false; // Initial state of the button
bool lastButtonState = false; // Previous state of the button

long UTC_offset = 0; // to suit how timeClient works
long BF_updateInt;   // ditto

// bool DONE button used to move to the title screen sequence in config.h
bool configCompleted = false;


// ------Default configuration values   this section is the WiFi Manager Parameter stuff------
// WiFi Manager Parameter - Global variables defaults
// Auth_B length should be max size + 1 = 126 
char Auth_B[126] = (""); // testing for BF not connected and release

// flag for saving wm data
bool shouldSaveConfig = false; // false per 1.2 using https://dronebotworkshop.com/wifimanager/
bool forceConfig = false; // made global was in setup
 

// setup wifi and get BF API Auth_B
WiFiManager wm;

void saveAuthToPreferences() {
  Preferences preferences;
  preferences.begin("auth", false); // Open preferences with the namespace "auth"
  preferences.putString("Auth_B", Auth_B); // Save Auth_B to preferences with the key "Auth_B"
  preferences.end(); // Close preferences
}

bool retrieveAuthFromPreferences() {
  Preferences preferences;
  preferences.begin("auth", true); // Open preferences in read-only mode
  String retrievedAuth = preferences.getString("Auth_B", "");
  preferences.end();
  retrievedAuth.toCharArray(Auth_B, sizeof(Auth_B));
  return true;
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
 
void configModeCallback(WiFiManager *myWiFiManager)
// Called when config mode launched
{
  Serial.println("Entered Configuration Mode");
 
  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
 
  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}
//Global variable from Prefs to use in main
bool SYD_DST = false; // extern bool in config.h
// bool BF_Poll;
bool wmWIPE = false; // ditto

// Get NTP Time and set the offset to UTC
// const long UTC_offset In Seconds = UTC_offset; // UTC_offset; 
// AEDT 39600 UTC+11:00  need to set offset for daylight saving time DST from touch
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_offset);

// This function is called to define the UTC Offset values - change these offsets to suit your location
void setUTCoffset() {
  Debug("");
  Debug("is syd dst active in setUTCoffset  "); Debugln(SYD_DST);
  Debug("");
  if (SYD_DST) {
    Serial.print("SYD_DST is true :  ");
    UTC_offset = 39600;
    
  } else {
    Serial.print("SYD_DST is false :  ");
    UTC_offset = 36000;
  }
  timeClient.setTimeOffset(UTC_offset); // Update timeClient with the new UTC_offset
  Debug("UTC_offset = "); Debugln(UTC_offset);
 } 

// not tested
// This function is to set the interval to send a GET request to BF  3 or 10 minutes
// probably not needed as Tap List not that dynamic?

void setBF_PollInterval() {
  Serial.println("");
  if (BF_Poll) {
    Serial.print("BF_Poll is true 3 minute interval  ");
    BF_updateInt = 180000; // 3 minutes
    // Serial.print("BF_Poll = "); Serial.println(BF_updateInt);
  } else {
    Serial.print("BF_Poll is false 10 minute interval  ");
    BF_updateInt = 600000;  // 10 minutes
    
  }
  Serial.print("BF_Poll = "); Serial.println(BF_updateInt);
 } 


unsigned long currentTime; // for BF Update in loop 

// these all need sorting out so that the variables from touch work
// ------------------------------------------------------------------------------------------------------------
/* Timing for LOOP functions NTP and accessing BF and screen refresh for  rotation  BPL set for 90 secs data may or may not have been updated yet from GM BPL BF
  "independant" timed events  https://www.programmingelectronics.com/arduino-millis-multiple-things/   */
const long eventTime_1_NTP = 1000;     // in ms 1 sec  
// const long eventTime_2_BF = BF_updateInt; // not used
const long eventTime_2_BF = 180000;     // in ms  1 min 1000 * 60  = 60000 180 = 3 mins note max 150 requests per hour changed to 60000 for testing

/* When did they start the race? */
unsigned long previousTime_1 = 0; // NTP
unsigned long previousTime_2 = 0; // BF


// BF HTTPS requests First set up authorisation - login details to BF API
// WiFiClientSecure client; // secure in TapList.h

// http://www.barth-dev.de/online/rgb565-color-picker/ // now in config.h

// Not used but could be for a more detailed listing 1 screen per beer?
// Function prototypes
void switchScreen();
// void screen1();
// void screen2();
// void screen3();
// void screen4();

// Global Screen variables 
/* const byte numScreens = 2; // NB if you get this number wrong you will have too many OR too few screens shown!
byte currentScreen = 0;  // Variable to store the current screen number
const int screenInterval =10000;  // 10 seconds
const int totalInterval = 90000;  // 90 seconds 
unsigned long previousMillis = 0;   */

// config screen Button instances  i.e.screen positioning and button names
// column 1 
Button button1(20, 40, 130, 25,  "    --     ");
Button button2(20, 80, 130, 25,  "    --     ");
Button button3(20, 120, 130, 25, "    --   ");
Button button4(20, 160, 130, 25, "  wm WIPE");
Button button5(20, 200, 130, 25, "  SYD_DST ");
// buttons column 2
Button button6(170, 40, 130, 25,   "    --  ");
Button button7(170, 80, 130, 25,   "    -- ");
Button button8(170, 120, 130, 25,  "    --  ");
Button button9(170, 160, 130, 25,  "    --  ");
Button button10(170, 200, 65, 25, " OK ");
Button button11(245, 200, 65, 25, "DONE" ); //  Config Completed also button for testing - print config

// function to handle button interactions in the loop
void handleButtonInteraction(ScreenPoint sp, Button &button, bool &setting, const char* settingName) {
  if (button.isClicked(sp)) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = millis();
      setting = !setting;// needed this
      
      buttonState = !buttonState;
      if (buttonState != lastButtonState) {
        lastButtonState = buttonState;
        setting = buttonState;
        Serial.print(settingName);
        Serial.print(": ");
        Serial.println(setting);
      }
    }
  }
} 



// Define a function to handle screen switching: // not used
// The switchScreen() function is called whenever the screen needs to be switched, 
// such as in the loop() function based on timing or external events. 
// It ensures that the correct screen function is called to update the display accordingly.

/* void switchScreen(int screenNumber) {
  // Perform actions based on the screenNumber value
  // tft.fillScreen(TFT_BLACK);
  
 switch (screenNumber) {
    case 0:
      // screen1();
      break;
    case 1:
      // screen2();
      break;
    case 2:
      // screen3();
      break;
    case 3:
      // screen4();
      break;
    default:
      // Handle invalid screenNumber value
      break;
  }
} */
    


//############################################################## setup
void setup() 
{
  
 
  // *****************************   SET TO false for auto operation   *************************************
  // needs to be here so you don't do touch cali every time or perhaps not
  bool forceConfig = false; // true takes us to the AP  true after reset launched AP put back to false for normal operation and works automatically after restart
  bool prefsSetup = retrieveAuthFromPreferences();
  if (!prefsSetup) 
  {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  // set up hostname for the router 
  WiFi.hostname(hostname); // just before WIFI.mode etc works
  WiFi.mode(WIFI_STA);     // explicitly set mode, esp defaults to STA+AP
  
  Serial.begin(115200);
  Serial.println(F(""));

  // LED setup
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  
  // Initially, turn off all LEDs
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, HIGH);
  
  /* // Define CYAN color (R: 0, G: 100, B: 100)
  analogWrite(CYD_LED_RED, 255);     // Turn off red 255 ie HIGH
  analogWrite(CYD_LED_GREEN, 155);  // Adjust green intensity (100/255 = 39% ,  255* 39% =~100 ,Inversion = 255 - 100 = 155)
  analogWrite(CYD_LED_BLUE, 155);   // Adjust blue intensity ( ditto )
   */
  
  
  // TFT Splash Screen
  tft.init();
  tft.setRotation(1);

  tft.fillScreen(TFT_GOLD);
  tft.fillRoundRect(4, 4, 312, 232, 2, TFT_BLACK);

  // open font render setup
  ofr.setDrawer(tft); // Link drawing object to tft instance (so font will be rendered on TFT)
  ofr.setFontColor(TFT_SILVER,TFT_BLACK);

    // Load the font and check it can be read OK
    if (ofr.loadFont(TTF_FONT, sizeof(TTF_FONT))) {
      Serial.println(F("Initialise error"));
      return;
    }
  
  // Set the cursor to top left
  ofr.setCursor(12, 10);
  ofr.setFontSize(30); // set font size 
  ofr.printf("TappO CYD-esp32\n");

  ofr.setCursor(105, 155); // was 105 145 CYD
  ofr.setFontSize(30); // set font size 
  ofr.printf("version  ");
  ofr.printf(version);

  
  ofr.setFontColor(TFT_MAGENTA, TFT_BLACK);
  ofr.setCursor(70, 70);
  ofr.setFontSize(120); // set font size
  ofr.printf("TappO\n");
  delay(3000); // view screen

  ofr.setFontColor(TFT_SILVER, TFT_BLACK);
  ofr.setCursor(70, 190);
  ofr.setFontSize(40); // set font size
  ofr.printf("Look for TappO_AP");

  delay(2000); 

   
  
  // ******************************************************************************************************* 
  // WiFiManager------------------------------------------------------------------------

  // Remove any previous network settings  for testing
  // wm.resetSettings(); // wipe settings every time REMEMBER for the PRODUCTION version to comment out!
  
  // set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback); // save the params
  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);
  // set a 3 minute timeout so the ESP doesn't hang waiting to be configured, for instance after a power failure,
  wm.setConfigPortalTimeout(180); // timeout if blocked too long and then the autoconnect should return Do a power cycle whenever you want to reconfigure.

  // Text box (String) BF Base64 encoded Authorisation Basic
  WiFiManagerParameter custom_text_box("BFAuth_text", "Enter the Base64 BF Authorization key for Basic Authorization", Auth_B, 126); // 125 == max length 125+1
  
  // WiFiManagerParameter custom_html("<br>"); // line break separator between the check boxes

  // add all your parameters here
  wm.addParameter(&custom_text_box); // Auth_B
    
  // just show the wifi not the other options
  std::vector<const char *> menu = {"wifi"}; // only show WiFi on the menu not the other stuff
  wm.setMenu(menu);                          // custom menu, pass vector

  // set dark theme
  wm.setClass("invert");

  // set static ip  dns  IPAddress(192,168,0,1  if you have more that one TappO or FermWatch make sure you use different IP addresses!
  //-------------------------*** AP naming also needs to be unique to each TappO or FermWatch------------------------------------------------
  // these are the defaults
  wm.setSTAStaticIPConfig(IPAddress(192, 168, 0, 100), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0), IPAddress(192, 168, 0, 1)); // set static ip,gw,sn,dns
  wm.setShowStaticFields(true);                                                                                                           // force show static ip fields
  wm.setShowDnsFields(true);     
  
  if (forceConfig)
    // Run if we need a configuration
  {
    if (!wm.startConfigPortal("TAPPO_AP", "")) // no pwd 
    {
      Serial.println("failed to connect and hit timeout");
      delay(2000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  }
  else
  {
    if (!wm.autoConnect("TAPPO_AP", ""))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }
    
  
  
  // If we get here, we are connected to the WiFi
  Debugln("");
  Debug("WiFi connected  ");
  Debug("IP address : ");
  Debugln(WiFi.localIP());
  
  // Copy the string value
  strncpy(Auth_B, custom_text_box.getValue(), sizeof(Auth_B));
  Debug("BF Authorisation Basic Input: ");
  Debugln(Auth_B);  
      
  // Save the custom parameters to Preferences
  if (shouldSaveConfig)
  {
    saveAuthToPreferences();
  }
  
  // check
  retrieveAuthFromPreferences(); //to retrieve Auth_B
  Debug("Auth_B from Prefs  "); Debugln(Auth_B);
  Debugln("");
  
  // purple LED here preferences retrieved
  digitalWrite(CYD_LED_RED, LOW);    // Turn on red
  digitalWrite(CYD_LED_GREEN, HIGH); // Turn off green
  digitalWrite(CYD_LED_BLUE, LOW);   // Turn on blue
  

  // WiFi Connected messages
  tft.fillScreen(TFT_GOLD);
  tft.fillRoundRect(4, 4, 312, 232, 2, TFT_BLACK);
  
  ofr.setFontSize(40); // set font size
  ofr.setCursor(20, 20);
  ofr.printf("WiFi Connected");
  ofr.setCursor(20, 45);// was 20,50
  ofr.printf("TappO IP  ");
  ofr.setFontColor(TFT_SILVER,TFT_BLACK);
  ofr.printf(WiFi.localIP().toString().c_str()); // https://forum.arduino.cc/t/how-to-manipulate-ipaddress-variables-convert-to-string/222693/15

  Debugln("");
  Debug("TappO IP  ");
  Debugln(WiFi.localIP().toString());
  delay(2000); // view screen
      
  // cyd2usb setup touch from
  // https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/2-TouchTest/2-TouchTest.ino
  // needs -DUSE_HSPI_PORT  in platformio.ini
  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();
  tft.setRotation(1); //This is the display in landscape

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);
  /* 
  // in touchCali.h
  // Perform calibration routine
  // calibrateTouchScreen(); // Perform calibration uncomment for testing
  
  float xCalM = 0.09;  // Typical  calibration values
  float xCalC = -18.75;
  float yCalM = 0.07;
  float yCalC = -18.16; */

  // if already calibrated skip calibration steps  
  preferences.begin("calibration", false);

  if (checkCalibrationInPreferences()) {
    isCalibrated = preferences.getBool("isCalibrated", false); // Retrieve isCalibrated flag from Preferences
    Serial.println("  ");
    Serial.print("Calibration found in Preferences 1 is true ");Serial.println(isCalibrated);
    useCalibrationData();
  // Print the stored data from Preferences
    printCalibrationData();

   } else {
    calibrateTouchScreen(); // Perform calibration
    saveCalibrationToPreferences(); // Save calibration and isCalibrated flag to Preferences
    Serial.println("Calibration performed and saved to Preferences");
    // small delay between cali and menu to prevent double click
    delay(200);
  }

  if (isCalibrated){
  // tft.fillScreen(TFT_BLACK);
  // tft.setTextColor(TFT_PURPLE,TFT_BLACK);
  // tft.setTextSize(2);
  // tft.setCursor (12,15);
  Debug(  "Calibrated");
  }
  
   
  // display the config screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor (12,15);
  tft.println("Select Boxes Touch OK to save and DONE to continue");
  
 // show the saved config state by highlighted buttons
      // Call function to retrieve configuration
      getConfigFromPreferences(); 
      // Render buttons based on retrieved settings
      button1.render(brewF); 
      button2.render(GM);  
      button3.render(FWD);
      button4.render(wmWIPE); 
      button5.render(SYD_DST); 
      button6.render(Fahr);
      button7.render(Plato);
      button8.render(Temp_Corr);
      button9.render(BF_Poll);

    // renders blank buttons no status  for OK and DONE
    button10.render(tft);button11.render(tft);
    // tft.fillScreen(TFT_BLUE);  // wipe screen
  /*  Blank buttons for testing
   button1.render(tft);button2.render(tft); button3.render(tft);button4.render(tft);button5.render(tft);
  button6.render(tft); button7.render(tft); button8.render(tft);button9.render(tft);
  button10.render(tft);button11.render(tft); */
  
  // goto start of Loop to process other functions post setup  
  
  } // end setup



unsigned long lastFrame = millis();

// LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOP

void loop() 
{ 
  unsigned long currentTime = millis(); // original needed for BF loop timing

  // Setup and Configuration Touch Screen stuff
  if (!configCompleted) 
  {     // software initiated reset bypasses config screen
        getrstDONEPrefs();
        ScreenPoint sp;
        // Check if DONE button is pressed or rstDONE from prefs = true to cater for a heap initiated reset
        if ((button11.isClicked(sp)) || (rstDONE)) {
         // Debug(" rstDONE = 1 ? "); Debugln(rstDONE);
          getConfigFromPreferences(); // get the bool SYD_DST value and BF_Poll                      
          
          setBF_PollInterval(); // Call function to set BF GET updates not tested or used here
          
          // deal with DST  
          setUTCoffset();
          timeClient.begin();       
          
          delay (3000); // view the screen
          
          // in TapList.h downloads and displays Completed Batches          
          getBFCompleted ();
          // Set the cursor to top left
              ofr.setCursor(6, 2);
              ofr.setFontColor(TFT_MAGENTA, TFT_BLACK);
              ofr.setFontSize(40); // set font size 
              ofr.printf("TappO\n");         
             
          
          // reset rstDONE to false to cater for an ordinary reset
          SetrstDONEFalsePrefs();

        configCompleted = true;
        }
              
      if (ts.touched()) 
      {
        TS_Point p = ts.getPoint();
        sp = getScreenCoords(p.x, p.y);
        // call a function with 'sp' and relevant button and settings info
        handleButtonInteraction(sp, button1, brewF, "brewF");
        handleButtonInteraction(sp, button2, GM, "GM");
        handleButtonInteraction(sp, button3, FWD, "FWD");
        // handleButtonInteraction(sp, button4, wmWIPE, "WIPE"); // not saved
        handleButtonInteraction(sp, button5, SYD_DST, "SYD_DST");
        handleButtonInteraction(sp, button6, Fahr, "Fahr");
        handleButtonInteraction(sp, button7, Plato, "Plato");
        handleButtonInteraction(sp, button8, Temp_Corr, "Temp_Corr");
        handleButtonInteraction(sp, button9, BF_Poll, "BF_Poll");
        
        // these are different use buttons
        // handleButtonInteraction(sp, button10, save_OK, "OK");
        // handleButtonInteraction(sp, button11, PS, "SYD_DST");

        if (button4.isClicked(sp)) {
          // If the button is touched, initiate debounce logic
          if ((millis() - lastDebounceTime) > debounceDelay) {
            // Capture the current time for debounce comparison
            lastDebounceTime = millis();
        // Toggle the button state
            buttonState = !buttonState;
            
            // Check if the state has changed
            if (buttonState != lastButtonState) {
              lastButtonState = buttonState; // Update the last state
              
              // Update wmWIPE based on the buttonState
              wmWIPE = buttonState;
              // saveConfigToPreferences(); donesnt save - as intended to be a 'one off'
              // clearPreferences(); // wipe auth, config and calibration
              wm.resetSettings(); 
              Debug("WM WIPE ");
              Debugln(wmWIPE);
            }
          }
        }  

              
        // Ok to save 
        if (button10.isClicked(sp)) {
          // If the button is touched, initiate debounce logic
          if ((millis() - lastDebounceTime) > debounceDelay) {
            // Capture the current time for debounce comparison
            lastDebounceTime = millis();
            
            // Toggle the button state
            buttonState = !buttonState;
            
            // Check if the state has changed
            if (buttonState != lastButtonState) {
              lastButtonState = buttonState; // Update the last state
              
              // Update OK based on the buttonState
              save_OK = buttonState;
              saveConfigToPreferences();

              // Print the current state (for testing)
              Debugln("Configuration saved!");
              //getConfigFromPreferences(); // Call function to retrieve configuration
              Debug("OK: ");
              Debugln(OK);
            }
          }
        }  
        // and DONE to continue also debug print config 
        if (button11.isClicked(sp)) 
        {
          // If the button is touched, initiate debounce logic
          if ((millis() - lastDebounceTime) > debounceDelay) {
            // Capture the current time for debounce comparison
            lastDebounceTime = millis();
            
            // Toggle the button state
            buttonState = !buttonState;
            
            // Check if the state has changed
            if (buttonState != lastButtonState) {
              lastButtonState = buttonState; // Update the last state
              // Print the current state (for testing)
              // Update based on the buttonState
              DONE = buttonState;
           
                      
              Serial.println("Configuration Details");
              // Update  based on the buttonState
              getConfigFromPreferences(); // Call function to retrieve configuration
              Debug("DONE  ");  // debug print
              Debugln(DONE);
              
              tft.fillScreen(Backgnd);
              tft.fillRoundRect(4, 4, 312, 232, 2, InnerBac); 
              
            }
        }  
          
      }    
          
          
          // Render the buttons based on their current states
          button1.setPressed(brewF);
          button1.render(tft);
          button2.setPressed(GM);
          button2.render(tft);
          button3.setPressed(FWD);
          button3.render(tft);
          button4.setPressed(wmWIPE);
          button4.render(tft);
          button5.setPressed(SYD_DST);
          button5.render(tft);
          button6.setPressed(Fahr);
          button6.render(tft);
          button7.setPressed(Plato);
          button7.render(tft);
          button8.setPressed(Temp_Corr);
          button8.render(tft);
          button9.setPressed(BF_Poll);
          button9.render(tft);
          button10.setPressed(OK);
          button10.render(tft);
          button11.setPressed(DONE);
          button11.render(tft);

      

      } // end touch and setup
  
     
  } else 
  { // Now run main program logic
        
      
      // CLOCK   Refresh NTP Time every 1 sec -----------------------------------------------------------------------------
      // https://github.com/RalphBacon/BB5-Moving-to-a-State-Machine/blob/main/Sketches/3-NonBlocking.ino
      
      {
        static unsigned long NTPMillis = millis();
        if (millis() - NTPMillis >= 1000) // 1 sec
        { timeClient.update();
          // Get the raw time as seconds since Jan 1 1970
          unsigned long rawTime = timeClient.getEpochTime();

          // Convert raw time to a 'time_t' type
          time_t currentTime = static_cast<time_t>(rawTime);

          // Convert 'time_t' to a 'tm' struct
          struct tm *timeinfo;
          timeinfo = localtime(&currentTime);

          // Format and print the day and date
          char formattedDay[20];
          strftime(formattedDay, sizeof(formattedDay), "%a %d %b", timeinfo);
          // Serial.printf("Day and date: %s\n", formattedDay);
          ofr.setFontColor(TFT_SILVER, TFT_BLACK);
          ofr.setCursor(120, 4);//was 
          ofr.setFontSize(30);//
          ofr.printf(formattedDay);

          // tft.fillScreen(TFT_BLACK);   // clear touch screen// test only
          
          
          tft.fillRect(214, 5, 100, 30, TFT_BLACK); // background for custom font  
          ofr.setCursor(254, 4);//was 214, 12 
          ofr.setFontSize(30);//about text 2 
          ofr.printf(timeClient.getFormattedTime().c_str());
          NTPMillis = millis();                         // We must reset the local millis variable
        }
      }

     
      // Not used
      // Get the current time 
      unsigned long currentMillis = millis(); 
      // Check if it's time to switch screens  // not used
      /* if (currentMillis - previousMillis >= screenInterval) {
        previousMillis = currentMillis;
        
        // Increment the current screen index
        currentScreen++;
        if (currentScreen >= numScreens) {
          currentScreen = 0;
        }
        
        // Switch to the next screen
        switchScreen(currentScreen); // 
      
      }      */
  
      // red LED ON if brewF = False
      if(brewF ==false)
      {
            // turn on the RED LED CYD
            digitalWrite(CYD_LED_BLUE, HIGH);
            digitalWrite(CYD_LED_RED, LOW);
            digitalWrite(CYD_LED_GREEN, HIGH);
      }

      if (brewF == true)
      {     // turn ON the GREEN LED CYD
            digitalWrite(CYD_LED_BLUE, HIGH);
            digitalWrite(CYD_LED_RED, HIGH);
            digitalWrite(CYD_LED_GREEN, HIGH); //turned off to save the battery!
      } 
      if (currentTime - previousTime_2 >= eventTime_2_BF) 
      {
       
          

              
          previousTime_2 = currentTime; // Update the timing



       }  
               
            
        

  }
      // Debugging
      // checkFreeHeap();

} // end LOOP


/* // To create purple (mixing red and blue):
  digitalWrite(CYD_LED_RED, LOW);    // Turn on red
  digitalWrite(CYD_LED_GREEN, HIGH); // Turn off green
  digitalWrite(CYD_LED_BLUE, LOW);   // Turn on blue
  delay(5000);
  
  // To create yellow (mixing red and green):
  digitalWrite(CYD_LED_RED, LOW);    // Turn on red
  digitalWrite(CYD_LED_GREEN, LOW);  // Turn on green
  digitalWrite(CYD_LED_BLUE, HIGH);  // Turn off blue

  delay(5000); */
  
/*  
For example, if you want to achieve an intensity of 100 out of 255 (which represents approximately 39% intensity), 
you'd need to calculate its inverted value within the 0-255 range. In this case:
Intensity = 100
Inverted intensity = 255 - Intensity
Inverted intensity = 255 - 100 = 155

This calculation gives you the value to set for the LED with active low logic to achieve an intensity equivalent to 100 
in the 0-255 range.
  */
