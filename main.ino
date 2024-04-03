#include <Adafruit_Fingerprint.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(2, 3);
SoftwareSerial sendData(-1,10); //only send through pin 10
#else
#define mySerial Serial1

#endif

#include <ezButton.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
RTClib rtc;
DateTime now;

const int SHORT_PRESS_TIME = 1000; // 1000 milliseconds

ezButton button1(7);
ezButton button2(8);  

//for using millis instead of delay for time
unsigned long currentTime=0;

unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool isPressing1,isPressing2 = false;
bool isLongDetected1,isLongDetected2 = false;
bool printed,intro = false;
bool option1,option2 = false;
long pressDuration = 0;
int assign = 0;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

void setup()
{
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.print("Initializing...");
  delay(1000);
  lcd.clear();
  
  Serial.begin(9600);
  sendData.begin(9600);
  button1.setDebounceTime(50);
  button2.setDebounceTime(50);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  
  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword()) {
    lcd.print("Sensor Detected");
  } else {
    lcd.print("Sensor Error");
    while (1) { delay(1); }
  }
  delay(1000);

  if(Serial.available()){
    String conn = Serial.readString();
    if(conn == "1"){
      lcd.clear();
      lcd.print("Connected to the");
      lcd.setCursor(3,1);
      lcd.print("internet.");
    }
  }
    
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
}

uint8_t readnumber(void) {
    uint8_t num = 0;
  
    while (num == 0) {
      while (! Serial.available());
      num = Serial.parseInt();
    }
    return num;
}

void loop()                     // run over and over again
{
  start:
  button1.loop(); // MUST call the loop() function first
  button2.loop();
  bool longPress1,longPress2 = false;
  bool shortPress1,shortPress2 = false;

  if(option1 == false && option2 == false)
    datetime();
   
  if(option1 == true)
    timein();

  if(option2 == true)
    timeout();
    
  if(button1.isPressed()){
    pressedTime = millis();
    isPressing1 = true;
    isLongDetected1 = false;
  }

  if(button1.isReleased()) {
    isPressing1 = false;
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if(pressDuration < SHORT_PRESS_TIME ){
      option1 = true;
      //Condition sa ESP32 na if Serial2.readString().toInt()>0, then pass ---> -1 ang ma pass if error
      timein();
      if(Serial.readString()=="200"){
        lcd.clear();
        lcd.print("Successfull");
      }
    }
  }

  if(button2.isPressed()){
    pressedTime = millis();
    isPressing2 = true;
    isLongDetected2 = false;
  }

  if(button2.isReleased()) {
    isPressing2 = false;
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if(pressDuration < SHORT_PRESS_TIME ){
      option2 = true;
      //Condition sa ESP32 na if Serial2.readString().toInt()>0, then pass ---> -1 ang ma pass if error
      timeout();
    }
  }
  
  if(isPressing1 == true && isLongDetected1 == false) {
    long pressDuration = millis() - pressedTime;

    if( pressDuration > SHORT_PRESS_TIME ) {
      isLongDetected1 = true;
      lcd.clear();
      lcd.print("Enrollment..");
      delay(1000);
      lcd.clear();
      lcd.print("Enroll in pc or");
      lcd.setCursor(0,1);
      lcd.print("reset to go back");
      Enroll();
    }
  }

  if(isPressing2 == true && isLongDetected2 == false) { //Intention was for Back but might change to Delete
    long pressDuration = millis() - pressedTime;

    if( pressDuration > SHORT_PRESS_TIME ) {
      isLongDetected2 = true;
      goto start;
    }
  }
  //CODE ABOVE IS FOR BUTTON AND ACTIONS
}

void datetime(){
  if(millis()-currentTime >= 1000){
    now = rtc.now();
    lcd.clear();
    displayDate();
    displayTime();

    currentTime = millis();
  }
}

void timein(){
  assign = getFingerprintIDez("Time-in user....", "Timed-in user #"); //Decimal is for discernment whether timein or timeout
  if(assign != 0){
    sendData.print(assign+0.1,1);
    dateTransmit();
  }
}

void timeout(){
  assign = getFingerprintIDez("Time-out user....", "Timed-out user#");
  if(assign != 0){
    sendData.print(assign+0.2,1);
    dateTransmit();
  }
}

void dateTransmit(){ //sends the date and time along with the user time-in/time-out
  sendData.print("*");
  sendData.print(now.month(),DEC);
  sendData.print('/');
  sendData.print(now.day(),DEC);
  sendData.print('/');
  sendData.print(now.year(),DEC);
  sendData.print("*");
  if(now.hour()<10)
    sendData.print("0");
  sendData.print(now.hour(),DEC);
  sendData.print(':');
  if(now.minute()<10)
    Serial.print("0");
  sendData.print(now.minute(),DEC);
  sendData.print(':');
  if(now.second()<10)
    Serial.print("0");
  sendData.println(now.second(),DEC);
}

void displayDate()
{
  lcd.setCursor(0,0);
  lcd.print("Date:");
  lcd.print(now.month(),DEC);
  lcd.print('/');
  lcd.print(now.day(),DEC);
  lcd.print('/');
  lcd.print(now.year(),DEC);
}

void displayTime()
{
  lcd.setCursor(0,1);
  lcd.print("Time:");

  if(now.hour()<10)
    lcd.print("0");
  lcd.print(now.hour(),DEC);
  lcd.print(':');
  if(now.minute()<10)
    lcd.print("0");
  lcd.print(now.minute(),DEC);
  lcd.print(':');
  if(now.second()<10)
    lcd.print("0");
  lcd.print(now.second(),DEC);
} 

//Enrolling Fingerprint into the microcontroller
void Enroll()
{
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }
  Serial.print("Enrolling ID #");
  Serial.println(id);

  while (!getFingerprintEnroll());
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      if(printed == false){
        Serial.print(".");
        printed = true;
      }
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
  printed = true;
  return true;
}
//Enroll

// returns 0 if failed, otherwise returns ID #
int getFingerprintIDez(String text, String check) {
  
  if(printed == false){
    lcd.clear();
    lcd.print("Fingerprint scan");
    lcd.setCursor(0,1);
    lcd.print(text);
    printed = true;
  }

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return 0;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return 0;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return 0;
  
  // found a match!
  int id = finger.fingerID;
  lcd.clear();
  lcd.print(check);
  lcd.setCursor(15,0);
  lcd.print(id);
  if(millis()-currentTime >=1000){
    printed = false;
    intro = false;
    option1 = false; option2 = false;
    currentTime = millis();
  }
  
  return finger.fingerID;
}

//Send integers to discern if timeout or timein
