/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-datalogging-google-sheets/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Adapted from the examples of the Library Google Sheet Client Library for Arduino devices: https://github.com/mobizt/ESP-Google-Sheet-Client
*/

#include <WiFi.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>
#include <HardwareSerial.h>
#include "secret.h" 
#define LED_PIN 4 

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;

// Token Callback function
void tokenStatusCallback(TokenInfo info);

//Hardware Serial for receiving messages from Arduino
HardwareSerial mySerial(2);

String user="";
String stats="";
String date="";
String timeTransmit="";
String Final = "";

void blinkLED(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(duration);
    digitalWrite(LED_PIN, LOW);
    delay(duration);
  }
}

void setup(){
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(9600);
    Serial.println();
    Serial.println();
   // mySerial.begin(9600,SERIAL_8N1,16);
   // mySerial.setTimeout(10);
   esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_BROWNOUT) {
    // Blink LED to indicate brownout error
    blinkLED(3, 1000); // Blink LED 3 times with 500ms delay
    Serial.println("Brownout detected!");
  }

    WiFi.mode(WIFI_STA);
    GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);
    
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(WIFI_SSID);
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        Serial.print(".");
        delay(1000);     
     } 
    Serial.println("\nConnected.");
    }
    Serial.println(1);
    Serial.println();
    
    configTime(0, 0, "pool.ntp.org");
    // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);

    // Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop(){
    // Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();
    
    if(!ready)
      blinkLED (3,100);
    else
      digitalWrite(LED_PIN, HIGH);
    Get_String();
    if (ready && Final!=""){
        String_Analyze(Final);
        lastTime = millis();
        
        FirebaseJson response;

        Serial.println("\nAppend spreadsheet values...");
        Serial.println("----------------------------");

        FirebaseJson valueRange;

        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", date);
        valueRange.set("values/[1]/[0]", timeTransmit);
        valueRange.set("values/[2]/[0]", user);
        valueRange.set("values/[3]/[0]", stats);

        // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
        // Append values to the spreadsheet
        bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
        if (success){
            response.toString(Serial, true);
            valueRange.clear();
        }
        else{
            Serial.println(GSheet.errorReason());
        }
        Serial.println();
        Serial.println(ESP.getFreeHeap());

        Final="";
        user="";
        stats="";
        date="";
        timeTransmit="";
    }
}

void Get_String() {
  if(Serial.available()>0) {
    Final = Serial.readString();
    digitalWrite(LED_PIN, LOW);
  }
}

void String_Analyze(String input) {
 // Separate first numbers for user# and type 
  int dotIndex = input.indexOf('.');
  int dateIndex = input.indexOf('*') + 1;
  int lastIndex = input.lastIndexOf('*');
  
  user = input.substring(0, dotIndex); // Extract "2"
  stats = input.substring(dotIndex + 1,dateIndex-1); // Extract "1"
  
  if(stats == "1")
    stats = "IN";
  else
    stats = "OUT";
    
  // Separate date and time
  date = input.substring(dateIndex, lastIndex); // Extract date
  timeTransmit = input.substring(lastIndex + 1); // Extract time
}

void tokenStatusCallback(TokenInfo info){
    if (info.status == token_status_error){
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}
