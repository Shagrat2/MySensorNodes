#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

#define MY_OTA_FIRMWARE_FEATURE
 
// load user settings
#include "config.h"
// load MySensors library
#include <MySensors.h>
// load NodeManager library
#include "NodeManager.h"
#include <avr/wdt.h>

// create a NodeManager instance
NodeManager nodeManager;

// before
void before() {
  wdt_disable();

  // Disable the ADC by setting the ADEN bit (bit 7) to zero.
  ADCSRA = ADCSRA & B01111111;
  
  // Disable the analog comparator by setting the ACD bit
  // (bit 7) to one.
  ACSR = B10000000;
  
  // setup the serial port baud rate
  Serial.begin(MY_BAUD_RATE);  
  /*
   * Register below your sensors
  */

  nodeManager.setBatteryMin(4.7);
  nodeManager.setBatteryMin(2.7);  
  nodeManager.setSleep(SLEEP,15,MINUTES);
  nodeManager.setSleepBetweenSend(200);
  nodeManager.setPowerPins(4,3,500);

  int sDHT = nodeManager.registerSensor(SENSOR_DHT22, 2);
  SensorDHT* DHTSensor = ((SensorDHT*)nodeManager.getSensor(sDHT));
  DHTSensor->setTackLastValue(true);

  /*
   * Register above your sensors
  */
  nodeManager.before();
}

// presentation
void presentation() {
  // Send the sketch version information to the gateway and Controller
	sendSketchInfo(SKETCH_NAME,SKETCH_VERSION);
  // call NodeManager presentation routine
  nodeManager.presentation();

}

// setup
void setup() {
  // call NodeManager setup routine
  nodeManager.setup();

  wdt_enable(WDTO_8S);
}

// loop
void loop() {
  wdt_reset(); 
  
  // call NodeManager loop routine
  nodeManager.loop();

}

// receive
void receive(const MyMessage &message) {
  // call NodeManager receive routine
  nodeManager.receive(message);
}


