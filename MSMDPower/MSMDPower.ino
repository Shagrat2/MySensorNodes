#define MY_PARENT_NODE_ID AUTO
#define MY_REPEATER_FEATURE // Enable repeater functionality for this node

// Flash options
#define MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FLASH_SS 7
#define M25P40 // Flash type

//#define MY_SIGNING_SOFT
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
#define MY_SIGNING_ATSHA204
#define MY_SIGNING_ATSHA204_PIN 16 // A2

//#define MY_SIGNING_REQUEST_SIGNATURES

// Enable debug prints to serial monitor
#define MY_DEBUG 
#define DEBUG

// Enables repeater functionality (relays messages from other nodes)
// #define MY_REPEATER_FEATURE

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
//#define MY_RFM69_FREQUENCY RF69_433MHZ

#ifdef M25P40
  #define MY_OTA_FLASH_JDECID 0x2020
#endif

#include <SPI.h>
#include <MySensors.h>

#ifdef M25P40
  #define SPIFLASH_BLOCKERASE_32K   0xD8
#endif

#define RELAY_ON   1 // GPIO value to write to turn on attached relay
#define RELAY_OFF  0 // GPIO value to write to turn off attached relay

#define CH1_LIGHT_PIN  3 // Relay 1
#define CH1_LIGHT_ID   1 // Relay sensor 1

#define CH2_LIGHT_PIN  4 // Relay 2
#define CH2_LIGHT_ID   2 // Relay sensor 2

#define CURRENT_VperAmp 185 // 185=5A, 100=20A, 66=30A

#define CH1_CURRENT_PIN A6      // ACS712 pin
#define CH1_CURRENT_ID  3       // Sensor current ID

#define CH2_CURRENT_PIN A7      // ACS712 pin
#define CH2_CURRENT_ID  4       // Sensor current ID
#define CH2_CURRENT_VperAmp 185 // 185=5A, 100=20A, 66=30A

#define NODESYSTEM_ID 254

unsigned long SEND_FREQUENCY = 900000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 

unsigned long currentTime;
unsigned long lastSend;

uint8_t CH1_LightState;
uint8_t CH1_LightStateSendTry = 0;
unsigned long CH1_LightStateSendTime = 0;

uint8_t CH2_LightState;
uint8_t CH2_LightStateSendTry = 0;
unsigned long CH2_LightStateSendTime = 0;

uint8_t CH1_CURRENT_MaxLoad = 255;
uint8_t CH2_CURRENT_MaxLoad = 255;

#define SendTry 8
#define SendTryTimeOut 1000

// Error ststus
uint8_t ErrorTry = 0;
unsigned long ErrorTime = 0;
char ErrorMsg[10];

#define INT_MIN -32767
#define INT_MAX 32767

int CH1_RawLast = INT_MIN;
int CH1_RawMax = INT_MIN;
int CH1_RawCnt = 0;

int CH2_RawLast = INT_MIN;
int CH2_RawMax = INT_MIN;
int CH2_RawCnt = 0;

int8_t CH1_CURRENT_Offset = 0; // Zero
int8_t CH2_CURRENT_Offset = 0; // Zero

// setup
void before() {
  uint8_t val;
  
  wdt_disable();

  Serial.begin(115200);

  testMode();

  //== Light
  // Then set relay pins in output mode
  pinMode(CH1_LIGHT_PIN, OUTPUT);
  pinMode(CH2_LIGHT_PIN, OUTPUT);
  
  // Set relay to last known state (using eeprom storage) 
  CH1_LightState = loadState(CH1_LIGHT_ID)?RELAY_ON:RELAY_OFF;
  digitalWrite(CH1_LIGHT_PIN, CH1_LightState);

  CH2_LightState = loadState(CH2_LIGHT_ID)?RELAY_ON:RELAY_OFF;
  digitalWrite(CH2_LIGHT_PIN, CH2_LightState);

  //== Current
  pinMode(CH1_CURRENT_PIN, INPUT);
  pinMode(CH2_CURRENT_PIN, INPUT);

  //== Read offset 
  // CH1
  val = loadState(CH1_CURRENT_ID*10+0);
  if (val != 0xFF){
    CH1_CURRENT_Offset = int8_t(val-128);
    #ifdef DEBUG
      Serial.print("Load current offset CH1: ");
      Serial.println(CH1_CURRENT_Offset);
    #endif
  }

  // CH2
  val = loadState(CH2_CURRENT_ID*10+0);
  if (val != 0xFF){
    CH2_CURRENT_Offset = int8_t(val-128);
    #ifdef DEBUG
      Serial.print("Load current offset CH2: ");
      Serial.println(CH2_CURRENT_Offset);
    #endif
  }

  //== Read max load
  // CH1
  val = loadState(CH1_CURRENT_ID*10+2);
  if (val != 0xFF){
    CH1_CURRENT_MaxLoad = val;
    #ifdef DEBUG
      Serial.print("Current max load CH1: ");
      Serial.println(CH1_CURRENT_MaxLoad);
    #endif
  }

  // CH2
  val = loadState(CH2_CURRENT_ID*10+2);
  if (val != 0xFF){
    CH2_CURRENT_MaxLoad = val;
    #ifdef DEBUG
      Serial.print("Current max load CH2: ");
      Serial.println(CH2_CURRENT_MaxLoad);
    #endif
  }

    // Make sure that ATSHA204 is not floating
//  pinMode(ATSHA204_PIN, INPUT);
//  digitalWrite(ATSHA204_PIN, HIGH); 

   #ifdef MY_OTA_FIRMWARE_FEATURE  
    #ifdef DEBUG
      Serial.println("OTA FW update enabled");
    #endif
  #endif

  wdt_enable(WDTO_8S);    

  lastSend=millis();
}

void setup() { 
  // After init MySensor 
}

void presentation()  
{   
  // Node info
  sendSketchInfo("MSMD Power", "1.0.1");

  // Light
  present(CH1_LIGHT_ID, S_LIGHT, "Light relay CH1");
  present(CH2_LIGHT_ID, S_LIGHT, "Light relay CH2");

  // Current
  present(CH1_CURRENT_ID, S_MULTIMETER, "Source current CH1");
  present(CH2_CURRENT_ID, S_MULTIMETER, "Source current CH2");
}

// loop
void loop() {
  float fval;
  int Raw;
  
  wdt_reset();

  currentTime = millis();

  //=== Current
  // CH1
  Raw = analogRead(CH1_CURRENT_PIN);
  if (Raw > CH1_RawMax) CH1_RawMax = Raw;
  CH1_RawCnt++;
  if (CH1_RawCnt > 1000){
    CH1_RawLast = CH1_RawMax;
    CH1_RawMax = INT_MIN;
    CH1_RawCnt = 0;

    if (CH1_LightState == RELAY_ON){
      // Source is low
      if (CH1_RawLast < 1){              
        strlcpy(ErrorMsg, "No load CH1", sizeof(ErrorMsg));
        ErrorTry = 5;
        ErrorTime = 0;
      }
/*
      // Owerload
      if (CH1_RawLast > CURRENT_MaxLoad*2) {        
        CH1_LightState = RELAY_OFF;
        digitalWrite(LIGHT_PIN, CH1_LightState);

        strlcpy(ErrorMsg, "Overload CH1", sizeof(ErrorMsg));
        ErrorTry = 200;
        ErrorTime = 0;
      }
*/      
    }
  }

  // CH2
  Raw = analogRead(CH2_CURRENT_PIN);
  if (Raw > CH2_RawMax) CH2_RawMax = Raw;
  CH2_RawCnt++;
  if (CH2_RawCnt > 1000){
    CH2_RawLast = CH2_RawMax;
    CH2_RawMax = INT_MIN;
    CH2_RawCnt = 0;

    if (CH2_LightState == RELAY_ON){
      // Source is low
      if (CH2_RawLast < 1){              
        strlcpy(ErrorMsg, "No load CH2", sizeof(ErrorMsg));
        ErrorTry = 5;
        ErrorTime = 0;
      }
/*
      // Owerload
      if (CH2_RawLast > CURRENT_MaxLoad*2) {        
        CH2_LightState = RELAY_OFF;
        digitalWrite(LIGHT_PIN, CH2_LightState);

        strlcpy(ErrorMsg, "Overload CH2", sizeof(ErrorMsg));
        ErrorTry = 200;
        ErrorTime = 0;
      }
*/      
    }
  }

  //=== Light state to gate
  // CH1
  if ((CH1_LightStateSendTry != 0) && (currentTime - CH1_LightStateSendTime > SendTryTimeOut)) {
    if (SendData(0, CH1_LIGHT_ID, V_LIGHT, CH1_LightState?RELAY_ON:RELAY_OFF, true)) {
      CH1_LightStateSendTry = 0;
    }

    CH1_LightStateSendTime = currentTime;
  }

  // CH2
  if ((CH2_LightStateSendTry != 0) && (currentTime - CH2_LightStateSendTime > SendTryTimeOut)) {
    if (SendData(0, CH2_LIGHT_ID, V_LIGHT, CH2_LightState?RELAY_ON:RELAY_OFF, true)) {
      CH2_LightStateSendTry = 0;
    }

    CH2_LightStateSendTime = currentTime;
  }

    //=== Repeater
  // Send feq
  // Only send values at a maximum frequency or woken up from sleep  
  if (currentTime - lastSend > SEND_FREQUENCY){
    sendHeartbeat();

    // Current
    wait(50);
    SendData(0, CH1_CURRENT_ID, V_CURRENT, ReadCurrent(CH1_RawLast), 3);
    SendData(0, CH2_CURRENT_ID, V_CURRENT, ReadCurrent(CH2_RawLast), 3);
      
    lastSend=currentTime;
  }

  //=== Send error
  if (ErrorTry != 0)
    if (currentTime - ErrorTime > SendTryTimeOut){
      if (SendData(0, NODESYSTEM_ID, V_TEXT, ErrorMsg, true)){
        ErrorTry = 0;
      } else {
        ErrorTry--;
        ErrorTime = currentTime;
      }
    }
}
bool SendData(uint8_t Dest, uint8_t Sensor, uint8_t Type, const char* Value, bool Ack) {
  MyMessage answerMsg;
  mSetCommand(answerMsg, C_SET);
  answerMsg.setSensor(Sensor);
  answerMsg.setDestination(Dest);
  answerMsg.setType(Type);
  return send(answerMsg.set(Value), Ack); 
}

bool SendData(uint8_t Dest, uint8_t Sensor, uint8_t Type, int Value, bool Ack){
  MyMessage answerMsg;
  mSetCommand(answerMsg, C_SET);
  answerMsg.setSensor(Sensor);
  answerMsg.setDestination(Dest);
  answerMsg.setType(Type);
  if (Value == INT_MIN)
    return send(answerMsg.set(""), Ack);
  else
    return send(answerMsg.set(Value), Ack);
}

bool SendData(uint8_t Dest, uint8_t Sensor, uint8_t Type, float Value, uint8_t Decimals, bool Ack){
  MyMessage answerMsg;
  mSetCommand(answerMsg, C_SET);
  answerMsg.setSensor(Sensor); 
  answerMsg.setDestination(Dest);
  answerMsg.setType(Type);
  if (Value == NAN)
    return send(answerMsg.set(""), Ack);
  else
    return send(answerMsg.set(Value, Decimals), Ack);
}

float ReadCurrentVolt(int RawLast){
  if (RawLast == INT_MIN)
    return NAN;
  else {
    if (RawLast == INT_MIN) 
      return NAN;
    else
      return (2.5*RawLast) / 1024.0; // Gets you mV  
  }
}

float ReadCurrent(int RawLast){ 
  float Val = ReadCurrentVolt(RawLast);
  if (Val == NAN)
    return NAN;
  else
    return Val / CURRENT_VperAmp;
}

void receive(const MyMessage &message) {
  uint8_t Dest = message.sender;

  // Request
  if (mGetCommand(message) == C_REQ) {
     #ifdef DEBUG
       Serial.print("Incoming request from:");
       Serial.println(Dest);
       Serial.print(", for sensor: ");
       Serial.println(message.sensor);
     #endif
     switch (message.sensor) {
       case CH1_LIGHT_ID:
          SendData(Dest, CH1_LIGHT_ID, V_LIGHT, CH1_LightState, false);
          break;

       case CH2_LIGHT_ID:
          SendData(Dest, CH2_LIGHT_ID, V_LIGHT, CH2_LightState, false);
          break;
           
       case CH1_CURRENT_ID:
         switch (message.type) {
          // Vltage
          case V_VOLTAGE:              
            SendData(Dest, CH1_CURRENT_ID, V_VOLTAGE, ReadCurrentVolt(CH1_RawLast), 3, false);
            break;

          // Raw data
          case V_CUSTOM:              
            SendData(Dest, CH1_CURRENT_ID, V_CUSTOM, CH1_RawLast, false);
            break;

          // Offset
          case V_VAR1:
            SendData(Dest, CH1_CURRENT_ID, V_VAR1, CH1_CURRENT_Offset, false);
            break;

          // Max load
          case V_VAR3:
            SendData(Dest, CH1_CURRENT_ID, V_VAR1, CH1_CURRENT_MaxLoad, false);
            break;
                    
          default:
            SendData(Dest, CH1_CURRENT_ID, V_CURRENT, ReadCurrent(CH2_RawLast), 3, false);              
            break;
         }
         
         break;

       case CH2_CURRENT_ID:
         switch (message.type) {
          // Vltage
          case V_VOLTAGE:              
            SendData(Dest, CH2_CURRENT_ID, V_VOLTAGE, ReadCurrentVolt(CH2_RawLast), 3, false);
            break;

          // Raw data
          case V_CUSTOM:              
            SendData(Dest, CH2_CURRENT_ID, V_CUSTOM, CH2_RawLast, false);
            break;

          // Offset
          case V_VAR1:
            SendData(Dest, CH2_CURRENT_ID, V_VAR1, CH2_CURRENT_Offset, false);
            break;

          // Max load
          case V_VAR3:
            SendData(Dest, CH2_CURRENT_ID, V_VAR1, CH2_CURRENT_MaxLoad, false);
            break;
                    
          default:
            SendData(Dest, CH2_CURRENT_ID, V_CURRENT, ReadCurrent(CH2_RawLast), 3, false);              
            break;
         }
         
         break;       
    }

    return ;
  }
  
  //=== Light state  
  if (message.type==V_LIGHT) {
    // CH1  
    if (message.sensor == CH1_LIGHT_ID) {
       // Change relay state     
       CH1_LightState = message.getBool()?RELAY_ON:RELAY_OFF;
       digitalWrite(CH1_LIGHT_PIN, CH1_LightState);
       // Store state in eeprom
       saveState(CH1_LIGHT_ID, message.getBool());
  
       // If set state not from gate
       if (Dest != 0){
         CH1_LightStateSendTry = SendTry;
         CH1_LightStateSendTime = 0;       
       }
    }

    // CH2
    if (message.sensor == CH2_LIGHT_ID) {
       // Change relay state     
       CH2_LightState = message.getBool()?RELAY_ON:RELAY_OFF;
       digitalWrite(CH2_LIGHT_PIN, CH2_LightState);
       // Store state in eeprom
       saveState(CH2_LIGHT_ID, message.getBool());
  
       // If set state not from gate
       if (Dest != 0){
         CH2_LightStateSendTry = SendTry;
         CH2_LightStateSendTime = 0;       
       }
    }

    // Write some debug info
    #ifdef DEBUG
      Serial.print("Incoming change from: ");
      Serial.print(Dest);
      Serial.print(", for sensor: ");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println(message.getBool());
    #endif
  } 
  //=== Current state
  // CH1
  else if (message.sensor == CH1_CURRENT_ID){
      switch (message.type) {
        case V_VAR1:
          CH1_CURRENT_Offset = int8_t(message.getInt());     
          saveState(CH1_CURRENT_ID*10+0, uint8_t(128+CH1_CURRENT_Offset));
    
          #ifdef DEBUG
            Serial.print("Set current offset CH1: ");
            Serial.println(CH1_CURRENT_Offset);
          #endif  
          break;
          
        case V_VAR3:
          CH1_CURRENT_MaxLoad = int8_t(message.getInt());     
          saveState(CH1_CURRENT_ID*10+2, uint8_t(CH1_CURRENT_MaxLoad));
    
          #ifdef DEBUG
            Serial.print("Set current max load CH1: ");
            Serial.println(CH1_CURRENT_MaxLoad);
          #endif  
          break;
      }      
  }

  // CH2
  else if (message.sensor == CH2_CURRENT_ID){
      switch (message.type) {
        case V_VAR1:
          CH2_CURRENT_Offset = int8_t(message.getInt());     
          saveState(CH2_CURRENT_ID*10+0, uint8_t(128+CH2_CURRENT_Offset));
    
          #ifdef DEBUG
            Serial.print("Set current offset CH2: ");
            Serial.println(CH2_CURRENT_Offset);
          #endif  
          break;
          
        case V_VAR3:
          CH2_CURRENT_MaxLoad = int8_t(message.getInt());     
          saveState(CH2_CURRENT_ID*10+2, uint8_t(CH2_CURRENT_MaxLoad));
    
          #ifdef DEBUG
            Serial.print("Set current max load CH2: ");
            Serial.println(CH2_CURRENT_MaxLoad);
          #endif  
          break;
      }      
  }
  
  // TODO Send presentation to button nodes
  // TODO Command button key   
}

void testMode(){
  #ifdef DEBUG  
    Serial.println(F("Test finished"));
  #endif
}
