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

//#define MY_OTA_FIRMWARE_FEATURE
 
// load user settings
#include "config.h"
// load MySensors library
#include <MySensors.h>
// load NodeManager library
#include "NodeManager.h"
#include <avr/wdt.h>

#define SKETCH_NAME "Sprinkers"
#define SKETCH_VERSION "2.1.3"

// create a NodeManager instance
NodeManager nodeManager;

unsigned long SEND_FREQUENCY = 300000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 

unsigned long currentTime;
unsigned long lastSend;

// before
void before() {
  wdt_disable();
 
  // setup the serial port baud rate
  Serial.begin(MY_BAUD_RATE);  
  Serial.println("Start");
  /*
   * Register below your sensors
  */
  SensorRelay* R1 = nodeManager.get( nodeManager.registerSensor(SENSOR_RELAY,2) );
  SensorRelay* R2 = nodeManager.get( nodeManager.registerSensor(SENSOR_RELAY,3) );
  SensorRelay* R3 = nodeManager.get( nodeManager.registerSensor(SENSOR_RELAY,4) );
  SensorRelay* R4 = nodeManager.get( nodeManager.registerSensor(SENSOR_RELAY,5) );

  R1->setInitialValue(LOW);
  R2->setInitialValue(LOW);
  R3->setInitialValue(LOW);
  R4->setInitialValue(LOW);
  
  int sensor_ldr = nodeManager.registerSensor(SENSOR_DS18B20, 6);
  SensorDs18b20* DS = ((SensorDs18b20*)nodeManager.get(sensor_ldr));
  DS->setSamplesInterval(SEND_FREQUENCY);
  DS->setTackLastValue(false);

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
  lastSend=millis();    
}

// loop
void loop() {
  wdt_reset();

  currentTime = millis();
      
  // call NodeManager loop routine
  nodeManager.loop();

  // Send feq
  if (currentTime - lastSend > SEND_FREQUENCY){
    sendHeartbeat();     
    lastSend=currentTime;
  }
}

// receive
void receive(const MyMessage &message) {
  // call NodeManager receive routine
  nodeManager.receive(message);
}


