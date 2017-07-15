// Interrupt driven binary switch example with dual interrupts
// Author: Patrick 'Anticimex' Fallberg
// Connect one button or door/window reed switch between 
// digitial I/O pin 3 (BUTTON_PIN below) and GND and the other
// one in similar fashion on digital I/O pin 2.
// This example is designed to fit Arduino Nano/Pro Mini

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <EEPROM.h>

//#define SYS_BAT
#define SendSkipTry

#define SKETCH_NAME "Door/Window Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "2"

#define PRIMARY_CHILD_ID 3
#define SECONDARY_CHILD_ID 4
#define CHILD_ID_DEV 254

#define cMaxSmallTry 3

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3700 // full voltage (100%)

float tGain = 0.9338;
float tOffset = 290.7;

unsigned long SEND_FREQUENCY = (1*60*60*1000ul); // Minimum time between send (in milliseconds). We don't wnat to spam the gateway.
unsigned long SEND_FREQUENCY_NOTSEND = (8*1000ul); // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 

#define PRIMARY_BUTTON_PIN 2   // Arduino Digital I/O pin for button/reed switch
#define SECONDARY_BUTTON_PIN 3 // Arduino Digital I/O pin for button/reed switch

#if (PRIMARY_BUTTON_PIN < 2 || PRIMARY_BUTTON_PIN > 3)
#error PRIMARY_BUTTON_PIN must be either 2 or 3 for interrupts to work
#endif
#if (SECONDARY_BUTTON_PIN < 2 || SECONDARY_BUTTON_PIN > 3)
#error SECONDARY_BUTTON_PIN must be either 2 or 3 for interrupts to work
#endif
#if (PRIMARY_BUTTON_PIN == SECONDARY_BUTTON_PIN)
#error PRIMARY_BUTTON_PIN and BUTTON_PIN2 cannot be the same
#endif
#if (PRIMARY_CHILD_ID == SECONDARY_CHILD_ID)
#error PRIMARY_CHILD_ID and SECONDARY_CHILD_ID cannot be the same
#endif
 
double oldTemp = 0;
unsigned long SleepTime = 0;

uint8_t sendValue=2;
long sendtry=0;

uint8_t sendValue2=2;
long sendtry2=0;

long errsend=0;       
long lasterrsend=0;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(PRIMARY_CHILD_ID, V_TRIPPED);
MyMessage msg2(SECONDARY_CHILD_ID, V_ARMED);

MyMessage TempMsg(CHILD_ID_DEV, V_TEMP);
#ifndef SYS_BAT
  MyMessage BattMsg(CHILD_ID_DEV, V_VOLTAGE);
#endif

#ifdef SendSkipTry
  MyMessage ErrSend(CHILD_ID_DEV, V_VAR4); 
#endif

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

float GetTemp() {
  signed long resultTemp;
  float resultTempFloat;
  // Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(10);                           // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                 // Convert
  while (bit_is_set(ADCSRA,ADSC));
  resultTemp = ADCL;
  resultTemp |= ADCH<<8;
  resultTempFloat = (float) resultTemp * tGain - tOffset; // Apply calibration correction
  return resultTempFloat;
}

void SendDevInfo()
{
  //========= Temperature =============
  double fTemp = GetTemp();
  
  if (oldTemp != fTemp){
    Serial.print("Temp:");
    Serial.println(fTemp);
  
    if (!send(TempMsg.set(fTemp, 2))){
      oldTemp = 0;
    } else {
      oldTemp = fTemp;
    }
  }

  //========= Battery ============= 
  int batteryV = readVcc();
  if (batteryV != 0){
    Serial.print("BatV:");
    Serial.println(batteryV * 0.001);        
    
    #ifdef SYS_BAT
      int batteryPcnt = min(map(batteryV, MIN_V, MAX_V, 0, 100), 100);
      sendBatteryLevel( batteryPcnt );
    #else          
      send(BattMsg.set(batteryV*0.001, 2));   
    #endif    
  }
  
  #ifdef SendSkipTry
    if (lasterrsend != errsend){
      send(ErrSend.set(errsend));
      lasterrsend = errsend;
    }
  #endif
}

void setup()  
{  
  wdt_disable();
  
  // Init adress from pin
  if (analogRead(3) >= 1023){
    Serial.begin(115200);
    Serial.println("Init adress");
    
    for (int i=0;i<512;i++) {
      EEPROM.write(i, 0xff);
    } 
  }
  
  // Disable the ADC by setting the ADEN bit (bit 7) to zero.
  ADCSRA = ADCSRA & B01111111;
  
  // Disable the analog comparator by setting the ACD bit
  // (bit 7) to one.
  ACSR = B10000000;
  
  // Setup the buttons
  pinMode(PRIMARY_BUTTON_PIN, INPUT);
  pinMode(SECONDARY_BUTTON_PIN, INPUT);

  // Activate internal pull-ups
  digitalWrite(PRIMARY_BUTTON_PIN, HIGH);
  digitalWrite(SECONDARY_BUTTON_PIN, HIGH);

  wdt_enable(WDTO_8S);
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  present(PRIMARY_CHILD_ID, S_DOOR, "Open sensor");  
  present(SECONDARY_CHILD_ID, S_VIBRATION, "Vibration sensor");
  present(CHILD_ID_DEV, S_ARDUINO_NODE, "Arduino sensors");
}

// Loop will iterate on changes on the BUTTON_PINs
void loop() 
{
  wdt_reset();
  
  uint8_t value;  
  
  // Short delay to allow buttons to properly settle
  wait(5);
  
  // Door sensor
  value = digitalRead(PRIMARY_BUTTON_PIN);  
  if (value != sendValue) {
     // Value has changed from last transmission, send the updated value
     if (!send(msg.set(value==HIGH ? 1 : 0))){
       // Not send last state       
       if (sendtry < cMaxSmallTry){
         sendtry++;
         errsend++;
       }
     } else {
       sendValue = value;
       sendtry = 0;
     }
  }

  // Vibration sensor
  value = digitalRead(SECONDARY_BUTTON_PIN);  
  if (value != sendValue2) {
     // Value has changed from last transmission, send the updated value
     if (!send(msg2.set(value==HIGH ? 1 : 0))){
       // Not send last state
       if (sendtry2 < cMaxSmallTry){
         sendtry2++;
         errsend++;
       }
     } else {
       sendValue2 = value;
       sendtry2 = 0;
     }
  }

  // Sleep until something happens with the sensor
  // Sleep time
  if (
       ((sendtry >= 1) && (sendtry < cMaxSmallTry)) ||
       ((sendtry2 >= 1) && (sendtry2 < cMaxSmallTry))  
     ){
    SleepTime = SEND_FREQUENCY_NOTSEND;
    Serial.println("sleep small");
  } else {
    SleepTime = SEND_FREQUENCY;
    Serial.println("sleep long");
  }
  
  if ( smartSleep(PRIMARY_BUTTON_PIN-2, CHANGE, SECONDARY_BUTTON_PIN-2, CHANGE, SleepTime) == -1){
    // Device Info
    SendDevInfo();
  };
}
