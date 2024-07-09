/*
 NMEA Serial Multiplexer.
 This skecth is designed to be compatible with SUN-shield.
 The board layout of the shield can be found here:
 
  
 Receives from the hardware serial 1, 2, 3 and from software serial.
 Send to hardware serial 0.
 
 The circuit: 
 * RX is digital pin 11 (connect to TX of other device)
 * TX is digital pin 12 (connect to RX of other device)
 
 Note:
 Not all digital pins on the Mega 2560 and Mega ADK support change 
 interrupts, so only the following can be used for RX: 
 10, 11, 12, 13, 50, 51, 52, 53
 
 On Mega 2560 and Mega ADK ICSP is connected to 50 51 52.
 So is better not use this pins for SoftwareSerial if we want to
 attach for example a wifi shield
 http://arduino.cc/en/Main/ArduinoWiFiShield
 
 Furthermore pin 13 is connected to LED 13
 http://arduino.cc/en/Main/ArduinoBoardMega2560
 http://arduino.cc/en/Main/ArduinoBoardMegaADK
 
 Pin 10 seem to be used by some other shield so the best choice
 seem to use the pins 11 12

 */
#include <SoftwareSerial.h>

SoftwareSerial softSerial(11, 12); // RX, TX

String serial1Sentence = "";
String serial2Sentence = "";
String serial3Sentence = "";
String softSerialSentence = "";

void setup() {
  // Initialize Serial 0 to 57600
  // This port is used multiplex data recevived at 4800 from
  // hardware serial 1,2,3 and from software serial.
  // To support multiplexing with no deplay a rate of 19200 is enough, 
  // but higher if supported by receiver if also fine (or better)
  Serial.begin(57600);
  
  // Initialize hardware serial ports to be multiplexed
  Serial1.begin(4800);
  Serial2.begin(4800);
  Serial3.begin(4800);

  // Initialize SoftwareSerial port to be multiplexed
  softSerial.begin(4800);
  
}

void loop() {
  hardwareSerialEvent(Serial1, serial1Sentence);
  hardwareSerialEvent(Serial2, serial2Sentence);
  hardwareSerialEvent(Serial3, serial3Sentence);
  softSerialEvent(softSerial, softSerialSentence);
}

void hardwareSerialEvent(HardwareSerial &serial, String &sentence) {
  if (serial.available()) {
    char c = serial.read();
    analizeCharacter(c, sentence);
  } 
}

void softSerialEvent(SoftwareSerial &serial, String &sentence) {
  if (serial.available()) {
    char c = serial.read();
    analizeCharacter(c, sentence);
  }
}

void analizeCharacter(char c, String &sentence){
  if(c == '\n') {
    // Verify checksum
    if (!isChecksumValid(sentence)) {
      multiplexSentence(sentence);
    }
    
  } else {
    /* 
     * If there are some disturb on the received signal 
     * the \n character cannot arrive. We discard the
     * collected data and we start with a new fresh sentence.
     */
    if(c == '$'){
      sentence = "";
    }
    sentence += c;
  }
}



// Function to calculate the NMEA checksum
char calculateChecksum(const String& sentence) {
  int checksum = 0;
  for (int i = 1; i < sentence.length(); i++) {
    if (sentence[i] == '*') {
      break;
    }
    checksum ^= sentence[i];
  }
  return checksum;
}

// Function to check if the checksum is valid
bool isChecksumValid(const String& sentence) {
  int asteriskIndex = sentence.indexOf('*');
  if (asteriskIndex == -1 || asteriskIndex + 2 >= sentence.length()) {
    return false;
  }

  // Extract the checksum from the sentence
  String checksumStr = sentence.substring(asteriskIndex + 1, asteriskIndex + 3);
  char sentenceChecksum = strtol(checksumStr.c_str(), NULL, 16);

  // Calculate the checksum
  char calculatedChecksum = calculateChecksum(sentence);

  return sentenceChecksum == calculatedChecksum;
}


/*
NMEA 0183 Sentence Formats:

MWV - Wind Speed and Angle
-------------------------------------------
$--MWV, x.x, a, x.x, a, a*hh<CR><LF>
-------------------------------------------
Fields:
1. Wind Angle, 0 to 360 degrees
2. Reference, R = Relative, T = True
3. Wind Speed
4. Wind Speed Units, K/M/N
   K = Kilometers Per Hour
   M = Meters Per Second
   N = Knots
5. Status, A = Data Valid
6. Checksum

VWR - Relative Wind Speed and Angle
-------------------------------------------
$--VWR, x.x, a, x.x, N, x.x, M, x.x, K*hh<CR><LF>
-------------------------------------------
Fields:
1. Wind Direction, 0 to 360 degrees
2. Reference, R = Relative, L = Left
3. Speed, Knots
4. N = Knots
5. Speed, Meters Per Second
6. M = Meters Per Second
7. Speed, Kilometers Per Hour
8. K = Kilometers Per Hour
9. Checksum
*/

// Function to convert a VWR sentence to an MWV sentence
String convertVWRtoMWV(const String& vwrSentence) {
  // Verify that the sentence contains 'VWR'
  int vwrIndex = vwrSentence.indexOf("VWR");
  if (vwrIndex == -1 || vwrIndex > 3) { // VWR should be at position 3-5
    return "";
  }

  // Split the sentence into components
  int commaIndex1 = vwrSentence.indexOf(',', vwrIndex + 3);
  int commaIndex2 = vwrSentence.indexOf(',', commaIndex1 + 1);
  int commaIndex3 = vwrSentence.indexOf(',', commaIndex2 + 1);

  if (commaIndex3 == -1) {
    return "";
  }

  // Extract the wind angle and direction
  String windAngle = vwrSentence.substring(vwrIndex + 4, commaIndex1);
  char windDirection = vwrSentence.charAt(commaIndex1 + 1);

  // Extract the wind speed in knots
  String windSpeedKnots = vwrSentence.substring(commaIndex2 + 1, commaIndex3);

  // Construct the MWV sentence
  String mwvSentence = "$IIMWV," + windAngle + "," + (windDirection == 'R' ? "R" : "L") + "," + windSpeedKnots + ",N,A";

  // Calculate and add the checksum
  char checksum = calculateChecksum(mwvSentence);
  mwvSentence += "*" + String(checksum, HEX) + "\r\n";

  return mwvSentence;
}

// Function to convert an MWV sentence to a VWR sentence
String convertMWVtoVWR(const String& mwvSentence) {
  // Verify that the sentence contains 'MWV'
  int mwvIndex = mwvSentence.indexOf("MWV");
  if (mwvIndex == -1 || mwvIndex > 3) { // MWV should be at position 3-5
    return "";
  }

  // Split the sentence into components
  int commaIndex1 = mwvSentence.indexOf(',', mwvIndex + 3);
  int commaIndex2 = mwvSentence.indexOf(',', commaIndex1 + 1);
  int commaIndex3 = mwvSentence.indexOf(',', commaIndex2 + 1);
  int commaIndex4 = mwvSentence.indexOf(',', commaIndex3 + 1);

  if (commaIndex4 == -1) {
    return "";
  }

  // Extract the wind angle
  String windAngle = mwvSentence.substring(mwvIndex + 4, commaIndex1);

  // Extract the reference (Relative/True)
  char reference = mwvSentence.charAt(commaIndex2 + 1);

  // Extract the wind speed in knots
  String windSpeedKnots = mwvSentence.substring(commaIndex3 + 1, commaIndex4);

  // Convert wind speed from knots to meters per second and kilometers per hour
  float speedKnots = windSpeedKnots.toFloat();
  float speedMetersPerSecond = speedKnots * 0.514444;
  float speedKilometersPerHour = speedKnots * 1.852;

  // Construct the VWR sentence
  String vwrSentence = "$IIVWR," + windAngle + "," + (reference == 'R' ? "R" : "L") + "," + windSpeedKnots + ",N,";
  vwrSentence += String(speedMetersPerSecond, 1) + ",M," + String(speedKilometersPerHour, 1) + ",K";

  // Calculate and add the checksum
  char checksum = calculateChecksum(vwrSentence);
  vwrSentence += "*" + String(checksum, HEX) + "\r\n";

  return vwrSentence;
}


void multiplexSentence(String &sentence){
  Serial.println(sentence);

  String vwrSentenceFromMWV = convertMWVtoVWR(sentence);
  if (!vwrSentenceFromMWV.isEmpty()) {
    Serial.println(sentence);
  }

  // Here you can do any other operation you need
  // - Log on SD
  // - Display Information on LCD (NMEA Lib needed)
  // - Send data through WIFI
  // - Send data through Bluethoot
  // - Elaborate the message
}
