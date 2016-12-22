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

#define SKETCH_NAME "Binary Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

unsigned long SEND_FREQUENCY = 5000;//900000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway.

#define INTERUPT_PIN 2   // Arduino Digital I/O pin for button/reed switch
#define BUTTON_PIN 3     // Arduino Digital I/O pin for button/reed switch

void setup(){   
  wdt_disable();
 
  // Disable the ADC by setting the ADEN bit (bit 7) to zero.
  ADCSRA = ADCSRA & B01111111;
  
  // Disable the analog comparator by setting the ACD bit
  // (bit 7) to one.
  ACSR = B10000000; 

  wdt_enable(WDTO_8S);
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
}

// Loop will iterate on changes on the BUTTON_PINs
void loop() 
{
  wdt_reset(); 
}
