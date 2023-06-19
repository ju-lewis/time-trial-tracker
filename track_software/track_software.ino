/*

Track Logger - Julian Lewis (2023)

Data fields:
0 - course code
1 - yacht code
2 - nominated speed

*/

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define fieldSelector 21
#define firstDigitBtn 19
#define secondDigitBtn 18
#define thirdDigitBtn 17
#define startBtn 16

const int codeOffset = 13;
const int rs = 14, en = 27, d4 = 26, d5 = 25, d6 = 33, d7 = 32;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Define initial variables
bool setupState = true;
int currField = 0;
bool changeFields = true;
bool changeDigits = false;
bool canStartLogging = true;
bool startOfRace = true;
bool errorHold = false;

char courseCode[] = "000";
char yachtCode[] = "000";
char nominatedSpeed[] = "000";

void incrementByDigit(char code[4], int changeDigits[3]);
void printErrorMessage();
int uploadGPSdata();

void setup() {
  // put your setup code here, to run once:
  
  // Define input button pinModes
  pinMode(fieldSelector, INPUT_PULLUP);
  pinMode(firstDigitBtn, INPUT_PULLUP);
  pinMode(secondDigitBtn, INPUT_PULLUP);
  pinMode(thirdDigitBtn, INPUT_PULLUP);
  pinMode(startBtn, INPUT_PULLUP);

  // Initialise LCD
  lcd.begin(16, 2);
  lcd.clear();
  
  // Print initial data field name
  lcd.print("Course: 000");
  Serial.begin(57600);

  // Initialise file system
  if(!SPIFFS.begin(true)){
  // Error has occurred
    printErrorMessage();
  }
  

}

void loop() {

  // We only want to read button inputs (OTHER THAN THE START/STOP button) in setupState
  if(setupState){
    // Constantly read buttons
    int fieldChange = 1 - digitalRead(fieldSelector);
    int incrementFirst = 1 - digitalRead(firstDigitBtn);
    int incrementSecond = 1 - digitalRead(secondDigitBtn);
    int incrementThird = 1 - digitalRead(thirdDigitBtn);

    int changedDigits[] = {incrementFirst, incrementSecond, incrementThird};
    
    // This will be true if any button has been pressed
    bool digButtonPressed = (incrementFirst || incrementSecond || incrementThird);

    // Change button pressed and change button is allowed to be pressed
    if(fieldChange == 1 && changeFields){
      // Disallow field changes until change button is released
      changeFields = false;
      currField++;

      // Detect and reset overflows
      if(currField > 2){
        currField = 0;
      }

      // ---------------JUST FOR DISPLAY -------------------//
      if(currField == 0){
        //Serial.printf("Now entering course code. (Currently %s)\n", courseCode);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Course: " + String(courseCode));
      } else if (currField == 1){
        //Serial.printf("Now entering yacht code. (Currently %s)\n", yachtCode);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Boat: " + String(yachtCode));
      } else {
        //Serial.printf("Now entering nominated speed. (Currently %s)\n", nominatedSpeed);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Speed: " + String(nominatedSpeed));
      }

    }
    // Detect change button depress and allow field changing again
    if(fieldChange == 0){
      changeFields = true;
    }

    // Check if a digit change button has been pressed and it can be pressed
    if(digButtonPressed && changeDigits == true){

      if(currField == 0){
        // Handle course code change
        incrementByDigit(courseCode, changedDigits);
        //Serial.printf("Course code: %s\n", courseCode);
        lcd.setCursor(0, 0);
        lcd.print("Course: " + String(courseCode));
        changeDigits = false;
      } else if(currField == 1) {
        // Handle yacht code change
        incrementByDigit(yachtCode, changedDigits);
        //Serial.printf("Yacht code: %s\n", yachtCode);
        lcd.setCursor(0, 0);
        lcd.print("Boat: " + String(yachtCode));
        changeDigits = false;
      } else if(currField == 2) {
        // Handle nominated speed change
        incrementByDigit(nominatedSpeed, changedDigits);
        //Serial.printf("Nominated speed: %s Knots\n", nominatedSpeed);
        lcd.setCursor(0, 0);
        lcd.print("Speed: " + String(nominatedSpeed));
        changeDigits = false;
      }
    }
    // Reset if the digit button hasn't been pressed
    if(!(digButtonPressed)) {
      changeDigits = true;
    }
    
  }
  int startLogging = 1 - digitalRead(startBtn);
  
  if(startLogging && canStartLogging) {
    //Serial.println("Setup complete, logging started.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Setup Complete.");
    lcd.setCursor(0, 1);
    lcd.print("Logging Started.");
    canStartLogging = false;
    // We now want to disable the LCD backlight and 5v input
    if(startOfRace){

      // Initialise gps data file to write
      File gpsFile = SPIFFS.open("/gpsData.txt", FILE_WRITE);
      if(!(gpsFile)) {
        printErrorMessage();
      }
      delay(1000);
      // Ensure serial is reading at the baud rate of the GPS module
      Serial.flush();
      Serial.begin(57600);
      // Loop while recording data
      while(true) {

        startLogging = 1 - digitalRead(startBtn);
        if(startLogging && canStartLogging) {
          // Stop button pressed
          gpsFile.close();
          // We want to re-enable LCD here.
          canStartLogging = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Finished Logging.");
          delay(1300);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Press again to");
          lcd.setCursor(0, 1);
          lcd.print("upload data.");
          // Wait until confirm button is pressed
          while(true) {
            startLogging = 1 - digitalRead(startBtn);
            if(startLogging && canStartLogging) {
              // Upload GPS data and hold program in a loop until disabled
              uploadGPSdata();
              while(true){};
            } else if (!(startLogging)){
              canStartLogging = true;
            }
            delay(100);
          }
        } else if (!(startLogging)){
          canStartLogging = true;
        }

        // Skip read if no data is available
        if (Serial.available() == 0) {
          continue;
        }
        
        String GPSsentence = Serial.readString();
        // Remove carriage return and line feed and log data
        GPSsentence.trim();
        gpsFile.println(GPSsentence);
      }
    }
    setupState = false;
  } else if (!(startLogging)) {
    canStartLogging = true;
  }

  delay(100);
}

void incrementByDigit(char code[], int changedDigits[3]) {

  // Iterate through all digits and increment
  for(int i = 0; i < 3; i++){
    int digit = code[i] - '0';

    // Detect and reset overflows
    if(digit + changedDigits[i] > 9){
      code[i] = '0';
    } else {
      code[i] = (digit + changedDigits[i]) + '0';
    }
  }
}

void printErrorMessage(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("An error has");
  lcd.setCursor(0, 1);
  lcd.print("occured. Restart");
  while(true){};
}

int uploadGPSdata() {

  // Prompt user to connect to a WiFi or hotspot
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning Wifis");
  int numNetworks = WiFi.scanNetworks();
  // Reinitialise LCD
  lcd.begin(16,2);
  if(numNetworks <= 0) {
    // If no networks are detected
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("None found.");
    lcd.setCursor(0, 1);
    lcd.print("Try again.");
    // Return error code
    return -1;
  }
  
  lcd.setCursor(0, 0);
  // Allow user to scroll through wifi networks
  int currNetworkNum = 0;
  int fieldChange;
  int selectBtn;
  bool fieldChangeAllowed = true;
  // Create 16 character buffer for printing
  char wifiName[16];
  // Print first network to screen
  snprintf(wifiName, 16, "%s", WiFi.SSID(0));
  lcd.clear();
  lcd.print(wifiName);
  while(true) {
    fieldChange = 1 - digitalRead(fieldSelector);
    selectBtn = 1 - digitalRead(startBtn);
    // Allow looping through available networks on button press
    if(fieldChange && fieldChangeAllowed) {
      fieldChangeAllowed = false;
      // Check if netID is within range and the SSID doesn't contain weird characters
      if(currNetworkNum + 1 < numNetworks)  {
        currNetworkNum++;
        lcd.clear();
        lcd.setCursor(0, 0);
        snprintf(wifiName, 16, "%s", WiFi.SSID(currNetworkNum));
        lcd.print(wifiName);
        
        
      } else {
        currNetworkNum = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        snprintf(wifiName, 16, "%s", WiFi.SSID(0));
        lcd.write(wifiName);
        
      }
    } else if (!(fieldChange)) {
      fieldChangeAllowed = true;
    }
    if(selectBtn) {
      // Network has been selected
      lcd.setCursor(0, 1);
      lcd.print("Enter password");
      delay(1500);
      lcd.clear();
      lcd.print("Enter password");
      lcd.setCursor(0, 1);
      // Define all possible characters
      char* characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
      char* numbers = "0123456789";
      char* symbols = ",.-_=+{}[]<>/?:;|";
      int charBtn;
      int numBtn;
      int symBtn;
      int nxtBtn;
      int submitBtn;
      int charIdx = 0;
      int numIdx = 0;
      int symIdx = 0;
      bool canType = true;
      int currPos = 0;
      bool buttonPressed = false;
      String currPassword = "";
      char currChar;
      // Enable user to enter password
      while(true) {
        charBtn = 1 - digitalRead(firstDigitBtn);
        numBtn = 1 - digitalRead(secondDigitBtn);
        symBtn = 1 - digitalRead(thirdDigitBtn);
        nxtBtn = 1 - digitalRead(fieldSelector);
        submitBtn = 1 - digitalRead(startBtn);
        buttonPressed = (charBtn || numBtn || symBtn || nxtBtn || submitBtn);

        if(buttonPressed && canType) {
          canType = false;
          // Set cursor to current write position
          lcd.setCursor(currPos, 1);
          // Handle cases for all buttons
          if(charBtn) {
            lcd.print(characters[charIdx]);
            currChar = characters[charIdx];
            if(charIdx + 1 < strlen(characters)) {
              charIdx++;
            } else {
              charIdx = 0;
            }
          }
          else if(numBtn) {
            lcd.print(numbers[numIdx]);
            currChar = numbers[numIdx];
            if(numIdx + 1 < strlen(numbers)) {
              numIdx++;
            } else {
              numIdx = 0;
            }
          }
          else if(symBtn) {
            lcd.print(symbols[symIdx]);
            currChar = symbols[symIdx];
            if(symIdx + 1 < strlen(symbols)) {
              symIdx++;
            } else {
              symIdx = 0;
            }
          }
          else if(nxtBtn) {
            currPassword += currChar;
            currPos++;
          } else if(submitBtn) {
            
            currPassword += currChar;
            lcd.setCursor(0, 0);
            lcd.print("Use password?   ");
            // Listen for yes/no input
            while(true) {
              charBtn = 1 - digitalRead(firstDigitBtn);
              symBtn = 1 - digitalRead(thirdDigitBtn);
              
              if(charBtn) {
                // yes
                lcd.clear();
                lcd.print("Connecting");
                Serial.printf("Connecting to %s with %s\n", wifiName, currPassword);
                // attempt 3 connections
                for(int i=0;i<3;i++) {
                  WiFi.begin(wifiName, currPassword);
                  delay(10000);
                  if(WiFi.status() == WL_CONNECTED) {
                    break;
                  }
                }

                if(WiFi.status() != WL_CONNECTED) {
                  lcd.clear();
                  lcd.print("Couldn't connect");
                  lcd.setCursor(0, 1);
                  lcd.print("Try again");
                  delay(1300);
                  lcd.clear();
                  lcd.print("Enter password");
                  lcd.setCursor(0, 1);
                  charIdx = 0;
                  numIdx = 0;
                  symIdx = 0;
                  currPos = 0;
                  currPassword = "";
                  break;
                }
                // We want to append WiFi password to file to remember passwords
                String savedPasswords;
                File initWifiFile = SPIFFS.open("wifiData.txt", FILE_READ);
                if(initWifiFile) {
                  // If the file exists read all saved passwords into memory
                  savedPasswords += initWifiFile.read();
                }
                initWifiFile.close();
                savedPasswords += (wifiName + "<|-|>" + currPassword);
                File newWifiFile = SPIFFS.open("wifiData.txt", FILE_WRITE);
                if(newWifiFile) {
                  newWifiFile.write(savedPasswords);
                }
                newWifiFile.close();

                // Now connect to server and upload data
                // Open file in read mode
                File gpsFile = SPIFFS.open("/gpsData.txt", FILE_READ);
                if(!(gpsFile)){
                printErrorMessage();
                }
                // Print status to LCD
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Uploading data");
                // Connect to TCP server
                WiFiClient client;
                int connection = client.connect("192.168.1.104", 12435);
                // Upload data if connection is successful
                if(connection) {
                  while(gpsFile.available()) {
                    String line = gpsFile.readStringUntil('\n');
                    client.println(line);
                    Serial.printf("Sent: %s\n", line);
                    
                  }
                  client.println();
                  client.stop();
                } else {
                  // Handle unsuccessful connection (tomorrow lol)
                  lcd.clear();
                  lcd.print("Couldn't connect");
                  lcd.setCursor(0, 1);
                  lcd.print("to server.");
                  delay(1000);
                }
                // Notify user data upload is complete, close file and socket
                delay(100);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Data upload");
                lcd.setCursor(0, 1);
                lcd.print("complete.");
                gpsFile.close();
                return 1;
                
              } else if(symBtn) {
                // no
                lcd.clear();
                lcd.print("Enter password");
                lcd.setCursor(0, 1);
                charIdx = 0;
                numIdx = 0;
                symIdx = 0;
                currPos = 0;
                currPassword = "";
                break;
              }
            }
          }
        } else if(!(buttonPressed)) {
          canType = true;
        }
        delay(50);
      }
    }
    delay(100);
  }
  
  return 1;
}