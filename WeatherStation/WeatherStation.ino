/*
NodeManager is intended to take care on your behalf of all those common tasks a MySensors node has to accomplish, speeding up the development cycle of your projects.

NodeManager includes the following main components:
- Sleep manager: allows managing automatically the complexity behind battery-powered sensors spending most of their time sleeping
- Power manager: allows powering on your sensors only while the node is awake
- Battery manager: provides common functionalities to read and report the battery level
- Remote configuration: allows configuring remotely the node without the need to have physical access to it
- Built-in personalities: for the most common sensors, provide embedded code so to allow their configuration with a single line 

Documentation available on: https://mynodemanager.sourceforge.io 
 */

// load user settings
#include "config.h"
 
// Enable debug prints to serial monitor
//#define MY_DEBUG  

#define SKETCH_NAME "NodeManagerTemplate"
#define SKETCH_VERSION "1.3"

#define SKETCH_NAME "Weather station"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"
 
// load MySensors library
#include <MySensors.h>
// load NodeManager library
#include "NodeManager.h"

// create a NodeManager instance
NodeManager nodeManager;

// before
void before() {
  // setup the serial port baud rate
  Serial.begin(MY_BAUD_RATE);  

  //nodeManager.setSleepMode(WAIT);
  nodeManager.setSleep(WAIT,5,SECONDS); 
  
  /*
   * Register below your sensors
  */ 
  
  nodeManager.registerSensor(SENSOR_BMP085);
  nodeManager.registerSensor(SENSOR_DHT22, 8);

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
  wdt_disable();
  
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


