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
    multiplexSentence(sentence);
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

void multiplexSentence(String &sentence){
  Serial.println(sentence);
  // Here you can do any other operation you need
  // - Log on SD
  // - Display Information on LCD (NMEA Lib needed)
  // - Send data through WIFI
  // - Send data through Bluethoot
  // - Elaborate the message
}
