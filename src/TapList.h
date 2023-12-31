#ifndef TAPLIST_H
#define TAPLIST_H


#include "touchCali.h" // has the preferences.h
#include "config.h" // has the variable names
#include "Button.h" // has the tft

#include <WiFiClientSecure.h> // to access the BF API
#include <ArduinoJson.h>
#include <vector>
#include "OpenFontRender.h"
OpenFontRender ofr;

// BF HTTPS requests First set up authorisation - login details to BF API
WiFiClientSecure client; // 
String receivedData;


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
// border colour
int16_t Backgnd = 0xFEA0; // gold
int16_t InnerBac = TFT_BLACK;



// for the BF API access
extern char Auth_B[126];


// reset
bool rstDONE;

// set DONE to true in Prefs to withstand reset in the heap code
void SetrstDONETruePrefs() 
{
      rstDONE = true;
      Preferences preferences;
      preferences.begin("config", false);
      preferences.putBool("rstDONE", rstDONE);
      preferences.end();
      
}
    // use in the in the post setup code in the loop
bool getrstDONEPrefs() 
{
      Preferences preferences;
      preferences.begin("config", true); // Open preferences in read-only mode
      rstDONE = preferences.getBool("rstDONE", false);
      preferences.end();
      // Serial.print("rstDONE: "); Serial.println(rstDONE); // debug only
      return true;
}

    // reset the rstDONE to false in Setup so that a normal reset will launch the config screen
void SetrstDONEFalsePrefs() 
{
      rstDONE = false;
      Preferences preferences;
      preferences.begin("config", false);
      preferences.putBool("rstDONE", rstDONE);
      preferences.end();
      
}

// structs to store all the received data, including measurements and other fields,
struct Measurement {
    const char* text;
    int value;
};

struct BrewData {
    int batchNo;
    float measuredFg;
    float measuredOg;
    const char* recipeName;
    std::vector<Measurement> measurements;
};

// Function to check if a string contains a substring used for variations in Tap texts
bool containsSubstring(const char* str, const char* substr) {
     return strstr(str, substr) != nullptr;
}


// Function to access the API and retrieve JSON data, filter JSON, Filter and sort received data
// print to screen
void getBFCompleted ()
{ 
  
      Debug("TapList Auth_B from  main  ");Debugln(Auth_B);
      // Debug("TapLlist BF API stuff status 0  = not set ");
      
      //************************** latest Tap updates  **************************************************
      Debugln("\nStarting connection to server...");
      
      client.setInsecure(); // the magic line, use with caution??? no certificate
      
      if (!client.connect("api.brewfather.app", 443))
      {
        Debugln("BF TapList Connection Failed!");
        
        // set rstDONE to true Prefs to withstand reset
        SetrstDONETruePrefs();
        Serial.println("Initiating restart.");
        ESP.restart();
               
        // return;
      }
      
      else
      {
        Debugln("Connected to BF API !");  
        
        // Send HTTP request using the html from Postman look for the Authorization Basic 
        // or do the base 64 encoded ASCII to Linux URLsafe encoding   https://www.base64encode.org/ 
        //

        // Make a HTTPS request: NB using fgEstimated does not cater for when FG is Fixed Final Gravity so use fg instead
        client.print("GET https://api.brewfather.app/v2/batches/?include=measuredOg,measuredFg,measurements,&status=Completed HTTP/1.1\r\n"); // made 1.1 and V2
        
        client.print("Host: api.brewfather.app\r\n"); // additional \r\n

        // Construct the Authorization header directly
        client.print("Authorization: Basic ");
        client.println(Auth_B);
        
        client.println("Connection: close\r\n\r\n");
        client.println();

        while (client.connected())
        {
          String line = client.readStringUntil('\n');
          if (line == "\r")
          {
            // Debugln("Authorised. Batch headers received");
            break;
          }
        }
        // if there are incoming bytes available
        // from the server, read them and print them:
        //while (client.available()) {
          //char c = client.read();
          //Serial.write(c);
          // }
        //client.stop();


        // Stream input; from Arduino JSON calculator
        // https://arduinojson.org/v6/assistant/#/step1   
        
        StaticJsonDocument<160> filter;

        JsonObject filter_0 = filter.createNestedObject();
        filter_0["batchNo"] = true;
        filter_0["measuredFg"] = true;
        filter_0["measuredOg"] = true;
        filter_0["recipe"]["name"] = true;

        JsonObject filter_0_measurements_0 = filter_0["measurements"].createNestedObject();
        filter_0_measurements_0["text"] = true;
        filter_0_measurements_0["value"] = true;


        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter)); // change input to client
        
          if (error) {
            Serial.print("deserializeJson() Brews Completed failed: ");
            Serial.println(error.f_str()); // was .c in some e.g.s
            return;
          }
          // Debugln("no deserialize error");
          
          // Define a vector to hold all filtered received brew data
          std::vector<BrewData> allBrewData;

          const int MAX_DOCUMENTS = 7; // Update according to your needs only tested to 7 mind.

          for (int i = 0; i < MAX_DOCUMENTS; ++i) 
          {
            JsonObject root = doc[i];

            if (root.isNull()) {
              break; // No more valid documents
            }
          
            // for debugging
            /* int batchNo = root["batchNo"];
            float measuredFg = root["measuredFg"];
            float measuredOg = root["measuredOg"];
            const char* recipeName = root["recipe"]["name"];
            
            Serial.println("");
            Serial.print(batchNo); Serial.print(" :  ");               
            Serial.print(recipeName);
            Serial.print(" ");            
            float abv = calculateABV(measuredFg, measuredOg);
            Serial.printf(": %.1f%%\n", abv); // Display ABV with 1 decimal place */
            
                BrewData brew;

                brew.batchNo = root["batchNo"];
                brew.measuredFg = root["measuredFg"];
                brew.measuredOg = root["measuredOg"];
                brew.recipeName = root["recipe"]["name"];

                // Extract measurements for this brew data
                JsonArray measurements = root["measurements"].as<JsonArray>();
                for (JsonObject measurement : measurements) 
                {
                    const char* measurementText = measurement["text"];
                    int measurementValue = measurement["value"];
                    // for debugging
                    /* Serial.print(measurementText);Serial.print(" :  ");
                    Serial.print("   ");
                    Serial.print(measurementValue);
                    Serial.println(""); */
                    brew.measurements.push_back({measurementText, measurementValue});
                }

                // Store this brew data in the vector
                allBrewData.push_back(brew);
          
                     
          }
//----------------------------------------------------------
                // this lot mostly the programy bits from ChatGPT after multiple questions!                
                            
                const int NUM_TAPS = 7;
                int tapPositions[NUM_TAPS] = {30, 60, 90, 120, 150, 180, 210};// screen spacing

                  // Clear the screen initially then fill with Tap Listing
                  tft.fillScreen(InnerBac);
                  ofr.setFontSize(35);
                  ofr.setFontColor(TFT_GOLD, TFT_BLACK);

                  int tapIndex = 0; // Track the displayed tap count

                  // Iterate through the brew data to extract Tap No, Name, and ABV for Taps 1 to 7 ignoring any 0
                  for (int tap = 1; tap <= 7; ++tap) 
                  {
                      // Find the brew corresponding to the current tap number
                      const auto& brew = std::find_if(allBrewData.begin(), allBrewData.end(), [tap](const BrewData& bd) 
                      {
                          for (const auto& measurement : bd.measurements) {
                              if (measurement.value == tap && containsSubstring(measurement.text, "Tap")) {
                                  return true;
                              }
                          }
                          return false;
                      });

                      // If brew is found and Tap No is not zero
                      if (brew != allBrewData.end() && brew->batchNo != 0) 
                      {
                          // Extract Name
                          const char* name = brew->recipeName;

                          // Calculate ABV that uses the UK Excise method that takes into account the alcohol
                          // https://www.gov.uk/government/publications/excise-notice-226-beer-duty/excise-notice-226-beer-duty--2#calculation-strength
                          // extract og and fg
                          float og = brew->measuredOg;
                          float fg = brew->measuredFg;
                          float Init_abv; // first calc ABV then work out the fudge factor 
                          Init_abv = ((og - fg) * 131.25); // nominal 131.25 bit high as 6% beers! seems to be what BF uses
                          Debugln("");
                            Serial.print(name);Serial.printf("  Initial ABV = %.3f\n", Init_abv); 

                            // Define the ABV ranges and their corresponding values
                            float ABV_ranges[] = {0.0, 3.3, 4.6, 6.0, 7.5, 9.0, 10.5, 12.0};
                            int ABV_values[] = {128, 129, 130, 131, 132, 133, 134};

                            // Find the appropriate ABV_adjG value based on Init_abv
                            int ABV_adjG = 0;
                            for (int i = 0; i < sizeof(ABV_ranges) / sizeof(ABV_ranges[0]); ++i) {
                                if (Init_abv >= ABV_ranges[i] && Init_abv <= ABV_ranges[i + 1]) {
                                    ABV_adjG = ABV_values[i];
                                    break;
                                }
                            }

                            Debug("Fudge : ");
                            Debugln(ABV_adjG);

                            // recalculate the ABV using the appropriate fudge factor 
                            float abv = ((og - fg) * ABV_adjG);
                            Serial.printf("Adjusted ABV   =  %.2f%s\n", abv, "%");
                          
                          // Display tap information for the first 7 taps
                          if (tapIndex < NUM_TAPS) 
                          {
                              // Set the cursor position for displaying tap information
                              ofr.setCursor(10, tapPositions[tapIndex]);
                              
                              // Display tap information
                              ofr.printf("%d  %s  %.1f%%", tap, name, abv);
                              
                              // Increment tap index for the next display
                              ++tapIndex;
                          }
                      }
                  }
                
        }
         
          
          
    client.stop();     
      
}      
          
   


#endif  // TAPLIST_H



  
