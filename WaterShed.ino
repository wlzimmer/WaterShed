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
int waiting= 0;

uint8_t advdata[] =
{
  0x02,  // length
  0x01,  // flags type
  0x06,  // le general discovery mode | br edr not supported
 
  0x02,  // length
  0x0A,  // tx power level
  0x04,  // +4dBm
 
  // if this variable block is not included, the RFduino iPhone apps won't see the device
  0x03,  // length
  0x03,  // 16 bit service uuid (complete)
  0x20,  // uuid low
  0x22,  // uuid hi
 
  0x14,  // max length 20
  0x09,  // complete local name type
  'T',
  'r',
  'a',
  'c',
  'k',
  'e',
  'r',
  ' ',
  ' ', // index 20: start 7 char space for custom name
  ' ', 
  ' ',
  ' ',
  ' ',
  ' ',
  ' ', // end 7 char space for custom name
  ' ',
  'v',
  '0',
  '2'
};
 
// select a flash page that isn't in use (see Memory.h for more info)
#define  MY_FLASH_PAGE  251
 
struct data_t
{
  // we will use java's famous 0xCAFEBABE magic number to indicate
  // that the flash page has been initialized previously by our
  // sketch
  int magic_number;
  int len;
  char custom_name[7];
};
 
struct data_t *flash = (data_t*)ADDRESS_OF_PAGE(MY_FLASH_PAGE);
 
//LibHumidity humidity = LibHumidity(0);
HTU21D myHumidity;

void setup() {
//  Serial.begin(9600);
//  Serial.println("Waiting for connection...");
  pinMode (OnOff, OUTPUT);
  digitalWrite (OnOff, HIGH);
//  RFduinoBLE.begin();

// Default address is 0x5A, if tied to 3.3V its 0x5B
// If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
//    Serial.println("MPR121 not found, check wiring?");
//    while (1);
  } //else Serial.println("MPR121 found!");
  pinMode(Light,INPUT);  
  RFduino_pinWakeCallback(Light, HIGH, myinthandler);
  waiting= 30*60*1000;
  
  if (flash->magic_number != 0xCAFEBABE) {
    flashSave(0, NULL);
  }
 
  Serial.begin(9600);
  startBLEStack();
  
}

void RFduinoBLE_onConnect() {
  flag = true;
  connected = 0;
//  Serial.println("Sending");
  // first send is not possible until the iPhone completes service/characteristic discovery
}

void RFduinoBLE_onDisconnect(){
    flag = false;
    waiting = .5*60*1000;
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
      if (waiting-- <= 0) {
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

void flashSave(int len, char *custom_name)
{
  flashPageErase(MY_FLASH_PAGE);
    
  struct data_t value;
  value.magic_number = 0xCAFEBABE;
  value.len = len;
  memcpy( value.custom_name, custom_name, len );
 
  flashWriteBlock(flash, &value, sizeof(value)); 
}
 
void startBLEStack()
{
//  printString("Recieved Flash: ", flash->custom_name, flash->len);
  int custom_name_len = flash->len;
  if (custom_name_len > 7) {
    custom_name_len = 7;
  }
  for(int i=0; i<custom_name_len; i++){
    advdata[20+i] = flash->custom_name[i];
  }
  
  // need to use the raw advertisment data approach because the more simple:
  // RFduinoBLE.deviceName didn't work with dynamically computed data
  RFduinoBLE_advdata = advdata;
  RFduinoBLE_advdata_len = sizeof(advdata);
  
  // start the BLE stack
  RFduinoBLE.begin();  
}
 
void RFduinoBLE_onReceive(char *data, int len)
{
//  printString("Recieved Data: ", data, len);
   if(len > 0 && data[0] == 'n'){
    // this is customized name setting
    
    // subtract off 'n'
    len = len - 1;
    data = data + 1;
 
    if(len > 7){
      len = 7;
    }
    flashSave(len, data);
 
    // restart the BLE stack so the new name shows up
    RFduinoBLE.end();
    startBLEStack();
    return;
  }
  if(len == 1 && data[0] == 'x'){
    // this is the command to turn it off
  }
}

void printString(char *label, char *data, int len)
{
  Serial.printf(label);
  for (int i=0; i<len; i++){
    Serial.printf("%c", data[i]);    
  }
  Serial.printf("\n");  
}
 

