/*
The data rate should be approximately:
  - 32 kbit/sec at 1.5ft (4000 bytes per second)
  - 24 kbit/sec at 40ft (3000 bytes per second)
    SCL -> Analog 5, SDA -> Analog 4  
*/

#include <RFduinoBLE.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "HTU21D.h"
#define Light 4
#define OnOff  3

Adafruit_MPR121 cap = Adafruit_MPR121();
volatile unsigned int pulsecount=0;

// flag used to start sending
int flag = false;
int connected=0;
int waiting=0;

//LibHumidity humidity = LibHumidity(0);
HTU21D myHumidity;

void setup() {
//  Serial.begin(9600);
//  Serial.println("Waiting for connection...");
  pinMode (OnOff, OUTPUT);
  digitalWrite (OnOff, HIGH);
  RFduinoBLE.begin();

// Default address is 0x5A, if tied to 3.3V its 0x5B
// If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
//    Serial.println("MPR121 not found, check wiring?");
//    while (1);
  } //else Serial.println("MPR121 found!");
  pinMode(Light,INPUT);  
  RFduino_pinWakeCallback(Light, HIGH, myinthandler);
//  attachInterrupt(0, myinthandler);
}

void RFduinoBLE_onConnect() {
  flag = true;
  connected = 0;
//  Serial.println("Sending");
  // first send is not possible until the iPhone completes service/characteristic discovery
}

void RFduinoBLE_onDisconnect(){
    digitalWrite (OnOff, LOW);
}

void loop() {    // generate the next packet
  if (flag) {
    if (connected++ > 30*60) digitalWrite (OnOff, LOW);

//    Serial.println(connected%2);
    while (! RFduinoBLE.sendInt(1024 + int(myHumidity.readTemperature()+.5))); 
    while (! RFduinoBLE.sendInt(0000 + int((myHumidity.readHumidity()+.5)*10))); 
//Serial.println ("temp="); Serial.println( myHumidity.readTemperature());
    while (! RFduinoBLE.sendInt(2048 + cap.filteredData(0)));
    while (! RFduinoBLE.sendInt(3072 + pulsecount));
    pulsecount = 0;
    delay(1000);
  } else {
      if (waiting++ > 5*60*1000) {
//        Serial.println("Sending Timeout");  
        digitalWrite (OnOff, LOW);
      }
      delay(1);
  }
}

int myinthandler(uint32_t ulPin) // interrupt handler
//void myinthandler() // interrupt handler
{
  pulsecount++;
}


