/*
NodeManager is intended to take care on your behalf of all those common tasks a MySensors node has to accomplish, speeding up the development cycle of your projects.

NodeManager includes the following main components:
- Sleep manager: allows managing automatically the complexity behind battery-powered sensors spending most of their time sleeping
- Power manager: allows powering on your sensors only while the node is awake
- Battery manager: provides common functionalities to read and report the battery level
- Remote configuration: allows configuring remotely the node without the need to have physical access to it
- Built-in personalities: for the most common sensors, provide embedded code so to allow their configuration with a single line 

Documentation available on: https://github.com/mysensors/NodeManager
 */
#include <avr/wdt.h>
 
// load user settings
#include "config.h"
// load MySensors library
#include <MySensors.h>
// load NodeManager library
#include "NodeManager.h"

// create a NodeManager instance
NodeManager nodeManager;

// before
void before() {
  wdt_disable(); 
  
  // setup the serial port baud rate
  Serial.begin(MY_BAUD_RATE);  

  nodeManager.setSleep(SLEEP,5,MINUTES);
  nodeManager.setBatteryInternalVcc(true);
  
  /*
   * Register below your sensors
  */  
  nodeManager.registerSensor(SENSOR_BME280);

  /*
   * Register above your sensors
  */
  nodeManager.before();

  wdt_enable(WDTO_8S);  
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


