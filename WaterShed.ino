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

// note that default max deviceName of 14 chars only allows a single advertisementData character
// so we use a length one character less so that we can have 2 chars advertisementData
char baseDeviceName [] = 
{
  'T',
  'r',
  'a',
  'c',
  'k',
  'e',
  'r',
  ' ',
  ' ',
  ' ',
  ' ',
  ' ',
  ' ',
  0
};


// this can only be a 2 char string is set in the advertisementData
#define  VERSION "03"

// select a flash page that isn't in use (see Memory.h for more info)
#define  MY_FLASH_PAGE  251

// this is currently 5 because that is all that will fit in the
// advertisment packet (along with the Tracker and version info)
#define  MAX_CUSTOM_NAME_LENGTH 5

struct data_t
{
  // we will use java's famous 0xCAFEBABE magic number to indicate
  // that the flash page has been initialized previously by our
  // sketch
  int magic_number;
  int len;
  char custom_name[MAX_CUSTOM_NAME_LENGTH];
};
 
struct data_t *flash = (data_t*)ADDRESS_OF_PAGE(MY_FLASH_PAGE);
 
HTU21D myHumidity;

void setup() {
  pinMode (OnOff, OUTPUT);
  digitalWrite (OnOff, HIGH);

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
 
  // Serial.begin(9600);
  println("Setup");
  startBLEStack();
  
}

void RFduinoBLE_onConnect() {
  println("onConnect");
  flag = true;
  connected = 0;
  // first send is not possible until the iPhone completes service/characteristic discovery
}

void RFduinoBLE_onDisconnect(){
    println("onDisconnect");
    flag = false;
    waiting = .5*60*1000;
}

void loop() {    // generate the next packet
  if (flag) {
    if (connected++ > 30*60) digitalWrite (OnOff, LOW);

    // These while loop causes the loop to block until something has subscribed
    // to the RFduino output characteristic (2221)
    while (! RFduinoBLE.sendInt(1024 + int(myHumidity.readTemperature()+.5))); 
    while (! RFduinoBLE.sendInt(0000 + int((myHumidity.readHumidity()+.5)*10))); 
    while (! RFduinoBLE.sendInt(2048 + cap.filteredData(0)));
    while (! RFduinoBLE.sendInt(3072 + pulsecount));
    pulsecount = 0;
    delay(1000);
  } else {
      if (waiting-- <= 0) {
        digitalWrite (OnOff, LOW);
      }
      delay(1);
  }
}

int myinthandler(uint32_t ulPin) // interrupt handler
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
  if (custom_name_len > MAX_CUSTOM_NAME_LENGTH) {
    custom_name_len = MAX_CUSTOM_NAME_LENGTH;
  }
  for(int i=0; i<custom_name_len; i++){
    baseDeviceName[8+i] = flash->custom_name[i];
  }
  if(custom_name_len < MAX_CUSTOM_NAME_LENGTH) {
    for(int i=custom_name_len; i<MAX_CUSTOM_NAME_LENGTH; i++){
      baseDeviceName[8+i] = ' ';
    }
  }

  RFduinoBLE.deviceName = baseDeviceName;

  RFduinoBLE.advertisementData = VERSION;
  
  // start the BLE stack
  RFduinoBLE.begin();  
}
 
void RFduinoBLE_onReceive(char *data, int len)
{
   println("onReceive");
   // printString(" data: ", data, len);
   if(len > 0 && data[0] == 'n'){
    // this is customized name setting
    
    // subtract off 'n'
    len = len - 1;
    data = data + 1;
 
    if(len > MAX_CUSTOM_NAME_LENGTH){
      len = MAX_CUSTOM_NAME_LENGTH;
    }
    flashSave(len, data);
 
    // restart the BLE stack so the new name shows up
    flag = false;
    RFduinoBLE.end();
    startBLEStack();
    return;
  }
  if(len == 1 && data[0] == 'x'){
    // this is the command to turn it off
    digitalWrite (OnOff, LOW);
  }
}

void println(const char *message)
{
  // Serial.println(message);
}

void printString(char *label, char *data, int len)
{
  Serial.printf(label);
  for (int i=0; i<len; i++){
    Serial.printf("%c", data[i]);    
  }
  Serial.printf("\n");  
}
 

