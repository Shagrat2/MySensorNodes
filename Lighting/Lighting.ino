#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO
#define MY_REPEATER_FEATURE // Enable repeater functionality for this node

// Flash options
#define MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FLASH_SS 7
#define MY_OTA_FLASH_JDECID 0x2013

//#define MY_SIGNING_SOFT
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
#define MY_SIGNING_ATSHA204
#define MY_SIGNING_ATSHA204_PIN 16 // A2

//#define MY_SIGNING_REQUEST_SIGNATURES

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enables repeater functionality (relays messages from other nodes)
// #define MY_REPEATER_FEATURE

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>

#define LIGHT_PIN  3 // Arduino Digital I/O pin number for first relay
#define LIGHT_ID   1 // State in eprom & MySensor node sensor
#define RELAY_ON   1 // GPIO value to write to turn on attached relay
#define RELAY_OFF  0 // GPIO value to write to turn off attached relay

#define TEMP_PIN   A3 // Termistor pin
#define TEMP_ID    2  // Sensor temp ID

#define CURRENT_PIN A6 // ACS712 pin
#define CURRENT_ID  3  // Sensor current ID
#define CURRENT_VperAmp 66 // 185=5A, 100=20A, 66=30A
#define CURRENT_Soffset -1 // Zero

unsigned long SEND_FREQUENCY = 900000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 

unsigned long currentTime;
unsigned long lastSend;
byte LightState;

void setup() { 
  wdt_disable();

  Serial.begin(115200);

  testMode();

  //== Light
  // Then set relay pins in output mode
  pinMode(LIGHT_PIN, OUTPUT);   
  // Set relay to last known state (using eeprom storage) 
  LightState = loadState(LIGHT_ID)?RELAY_ON:RELAY_OFF;
  digitalWrite(LIGHT_PIN, LightState);

  //== Temp
  pinMode(TEMP_PIN, INPUT);

  //== Current
  pinMode(CURRENT_PIN, INPUT);

  // Make sure that ATSHA204 is not floating
//  pinMode(ATSHA204_PIN, INPUT);
//  digitalWrite(ATSHA204_PIN, HIGH); 

  #ifdef MY_OTA_FIRMWARE_FEATURE  
    Serial.println("OTA FW update enabled");
  #endif

  wdt_enable(WDTO_8S);    
  lastSend=millis();  
}

void presentation()  
{   
  // Node info
  sendSketchInfo("Lighting", "1.0");

  // Light
  present(LIGHT_ID, S_LIGHT, "Light relay");

  // Temp
  present(TEMP_ID, S_TEMP, "Key temperature");

  // Current
  present(CURRENT_ID, S_MULTIMETER, "Source current");
}

void loop() 
{
  wdt_reset();

  currentTime = millis();
  
  //=== Repeater
  // Send feq
  // Only send values at a maximum frequency or woken up from sleep  
  if (currentTime - lastSend > SEND_FREQUENCY){
    sendHeartbeat();

    //TODO: Send temp flag
    SendTemp();

    //TODO: Send current flag
    SendCurrent();    
    
    lastSend=currentTime;
  }
}

float ReadTmp(){
  return analogRead(TEMP_PIN) * (5 / 1023.0);
}

void SendTemp(){
  MyMessage answerMsg;
  mSetCommand(answerMsg, C_SET);
  answerMsg.setSensor(TEMP_ID);
  answerMsg.setDestination(0);
  answerMsg.setType(V_TEMP);
  send(answerMsg.set( ReadTmp(), 1));
}


float ReadCurrent(){ 
  int RawValue = analogRead(CURRENT_PIN);
  Serial.println(RawValue);

  return (float)(RawValue-(512+CURRENT_Soffset))/1024*5/CURRENT_VperAmp*1000000;  
}

void SendCurrent(){
  MyMessage answerMsg;
  mSetCommand(answerMsg, C_SET);
  answerMsg.setSensor(CURRENT_ID);
  answerMsg.setDestination(0);
  answerMsg.setType(V_CURRENT);
  send(answerMsg.set( ReadCurrent(), 1));
}

void receive(const MyMessage &message) {

  // Request
  if (mGetCommand(message) == C_REQ) {
     MyMessage answerMsg;
     Serial.print("Incoming request for sensor: ");
     Serial.println(message.sensor);
     mSetCommand(answerMsg, C_SET);
     answerMsg.setSensor(message.sensor);
     answerMsg.setDestination(message.sender);
     switch (message.sensor) {
       case LIGHT_ID:
           answerMsg.setType(V_LIGHT);
           send(answerMsg.set(LightState));
           break;
       case TEMP_ID:
           SendTemp();           
           break;
       case CURRENT_ID:
           SendCurrent();
           break;
    }

    return ;
  }
  
  // Light state
  if ((message.type==V_LIGHT) && (message.sensor == LIGHT_ID)) {
     // Change relay state
     LightState = message.getBool()?RELAY_ON:RELAY_OFF;
     digitalWrite(LIGHT_PIN, LightState);
     // Store state in eeprom
     saveState(LIGHT_ID, message.getBool());
     
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
  } 
   // TODO Send presentation to button nodes
   // TODO Command button key
}

void testMode(){
  Serial.println(F("Test finished"));
}

