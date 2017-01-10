// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

#define SLEEP_TIME 300000 // Sleep time between reads (in milliseconds) 

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <EEPROM.h> 

#include <Wire.h>
#include "Adafruit_HTU21DF.h"

#define SKETCH_NAME "HTU21DF Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

#define HUM_ID 3
#define TEMP_ID 4

MyMessage msgHum(HUM_ID, V_HUM);
MyMessage msgTemp(TEMP_ID, V_TEMP);

float LastTemp = 0;
float LastHum = 0;

void setup()  
{  
  wdt_disable(); 

  Serial.begin(115200);
  
  // Init adress from pin
  if (analogRead(3) >= 1023){    
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

  if (!htu.begin()) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  
  wdt_enable(WDTO_8S);  
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  present(TEMP_ID, S_TEMP, "Temperature sensor");
  present(HUM_ID, S_HUM, "Humidity sensor");
} 

// Loop will iterate on changes on the BUTTON_PINs
void loop() 
{
  wdt_reset();

  float Temp = htu.readTemperature();
  float Hum = htu.readHumidity();

  if (LastTemp != Temp)
    if (send(msgTemp.set(Temp, 1)))
      LastTemp = Temp;
  
  if (LastHum != Hum)
    if (send(msgHum.set(Hum, 1)))
      LastHum = Hum; 

  sendHeartbeat();

  sleep( SLEEP_TIME );
}




