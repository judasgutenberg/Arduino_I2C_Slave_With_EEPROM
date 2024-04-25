/* Gus Mueller, April 25 2024
 * sends or receive data to a small Arduino (I use a Mini Pro at 8MHz) from a
 * NodeMCU or other master over I2C. 
 * Also accesses slave's EEPROM, which is great because I cannot get the pseudoEeprom code to work on my ESP8266
 * 
 */
#include "Wire.h"
#include <EEPROM.h>
#define I2C_SLAVE_ADDR 20
 
volatile int receivedValue = 0;
long lastMasterSignal = 0;
long millisNow = millis();
bool numericReturn = false;
byte numberOfCharsToReturn = 0;

char dataToSend[255]; //make it nice and long
long numericDataToSend = 0;
void setup(){
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onReceive(receieveEvent); 
  Wire.onRequest(requestEvent);
  Serial.begin(115200);
  //Serial.println("Starting up Arduino Slave...");
}

void loop(){
  //millisNow = millis();
  //Serial.print("millis: ");
  //Serial.print(millisNow);
  //Serial.print(" ");
  //Serial.print(lastMasterSignal);
  //Serial.println(" ");
  //delay(2000);
}

//send a bytes to the I2C master.  
void requestEvent(){
  //i only worry about longs for this to keep it simple
  if(numericReturn) {

    writeWireLong(numericDataToSend);
  } else {
    writeWholeArray((char *)dataToSend, numberOfCharsToReturn);
  }
}

//a data packet contains this data (with pipes delimitting byte boundaries):  COMMAND|destinationHigh|destinationLow|dataToWrite-OR-number of bytes to read
void receieveEvent() {
  lastMasterSignal = millisNow;
  //Serial.println("receive event");
  byte byteCount = 0;
  byte byteCursor = 0;
  byte receivedValues[4];
  byte receivedByte = 0;
  byte byteRead = 0;
  byte destinationHigh = 0;
  byte destinationLow = 0;
  unsigned long destination = 0;
  byte command = 0;
  byte amountToRead = 0;
  receivedValue = 0;
  while(0 < Wire.available()) // loop through all but the last
  {
    byteRead = Wire.read();
    if(byteCount == 0) { //command comes with the first byte
      command = byteRead;
      //Serial.println(command);
    } else if (byteCount == 1) {
      destinationHigh = byteRead;
    } else if (byteCount == 2) {
      destinationLow = byteRead;
    } else if (byteCount == 3 && command == 2) { //we're on byte 3 and it's read command, so we need to know how much data to read
      numberOfCharsToReturn = byteRead;
 
    } else {
      receivedByte = byteRead;
      //Serial.println("got more than a command");
      receivedValues[byteCursor] = receivedByte;
      //Serial.println(receivedByte);
      if(byteCount > 2) {
        
        destination = destinationHigh * 256 + destinationLow;
        if(command == 1) {
          EEPROM.update(destination + byteCursor, receivedValue);
        } else if (command == 2) {
          
        }
      }
      byteCursor++;
    }

    
    byteCount++;
  }
  
  for(byte otherByteCursor = byteCursor; otherByteCursor>0; otherByteCursor--) {
    receivedValue = receivedValue + receivedValues[otherByteCursor-1] * pow(256, byteCursor-1)  ;
    //Serial.println("qoot: ");
    //Serial.print(byteCursor-1);
    //Serial.print(":");
    //Serial.print(receivedValue);

  }
  //Serial.print("Destination: ");
  //Serial.print(destination);
  
  if(command == 1) { //we had an EEPROM write

  
  } else if (command == 5) { //we had a digital pin write
    //Serial.print("; value: ");
    //Serial.print(receivedValue);
    pinMode((int)destination, OUTPUT);
    if (receivedValue == 0 ) {
      digitalWrite((int)destination, LOW);
      //Serial.print(" SET LOW ");
    } else {
      digitalWrite((int)destination, HIGH);
      //Serial.print(" SET HIGH ");
    }
  } else if (command == 9) { //we had an analog pin write
      pinMode((int)destination, OUTPUT);
      analogWrite((int)destination, receivedValue - 256); //if you want to send analog content, add 256 to it first
  } else if (command == 2) { //we had an EEPROM read
    numericReturn = false;
    for(int i=0; i<numberOfCharsToReturn; i++){  //can send as many as 255 bytes at a time
      dataToSend[i] = EEPROM.read(destination+i);
    }
  } else if (command == 6) { //reading a digital pin
    pinMode((int)destination, INPUT);
    numericReturn = true;
    numericDataToSend = (long)digitalRead((int)destination);
  } else if (command == 8) { //reading an analog value
    pinMode((int)destination, INPUT);
    numericReturn = true;
    numericDataToSend = (long)digitalRead((int)destination);
  }
    //Serial.print("; Data sent: ");
    //Serial.print(dataToSend);
}
 
void writeWireLong(long val) {
  byte buffer[4];
  buffer[0] = val >> 24;
  buffer[1] = val >> 16;
  buffer[2] = val >> 8;
  buffer[3] = val;
  Wire.write(buffer, 4);
}

void writeWholeArray(char * buffer, byte numberOfCharsToReturn) {
  Wire.write(buffer, numberOfCharsToReturn);
}

 
 
