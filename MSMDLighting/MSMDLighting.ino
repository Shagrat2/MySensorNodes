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
#define DEBUG

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

#define TEMP_TERMISTORNOMINAL    10000 // Сопротивление при 25 градусах по Цельсию
#define TEMP_TEMPERATURENOMINAL  25    // temp. для номинального сопротивления (практически всегда равна 25 C)
#define TEMP_NUMAMOSTRAS         5     // сколько показаний используем для определения среднего значения 
#define TEMP_BCOEFFICIENT        3375  // бета коэффициент термистора (обычно 3000-4000)
#define TEMP_SERIESRESISTOR      10000

#define CURRENT_PIN A6      // ACS712 pin
#define CURRENT_ID  3       // Sensor current ID
#define CURRENT_VperAmp 185 // 185=5A, 100=20A, 66=30A

#define NODESYSTEM_ID 254

unsigned long SEND_FREQUENCY = 900000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 

unsigned long currentTime;
unsigned long lastSend;
uint8_t LightState;
uint8_t LightStateSendTry = 0;
unsigned long LightStateSendTime = 0;

uint8_t CURRENT_MaxLoad = 255;

#define SendTry 8
#define SendTryTimeOut 1000

#define INT_MIN -32767
#define INT_MAX 32767

int RawLast = INT_MIN;
int RawMax = INT_MIN;
int RawCnt = 0;
int8_t CURRENT_Offset = 0; // Zero

float TempLast = 0;
int TempMax = 60;

// Error ststus
uint8_t ErrorTry = 0;
unsigned long ErrorTime = 0;
char ErrorMsg[10];

void setup() { 
  uint8_t val;
  
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
  pinMode(12, INPUT);
  
  // Read offset  
  val = loadState(CURRENT_ID*10+0);
  if (val != 0xFF){
    CURRENT_Offset = int8_t(val-128);
    #ifdef DEBUG
      Serial.print("Load current offset: ");
      Serial.println(CURRENT_Offset);
    #endif
  }  

  // Read max load
  val = loadState(CURRENT_ID*10+2);
  if (val != 0xFF){
    CURRENT_MaxLoad = val;
    #ifdef DEBUG
      Serial.print("Current max load: ");
      Serial.println(CURRENT_MaxLoad);
    #endif
  }

  // Read max temp
  val = loadState(TEMP_ID*10+2);
  if (val != 0xFF){
    TempMax = val;
    #ifdef DEBUG
      Serial.print("Temp max max load: ");
      Serial.println(TempMax);
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
  float fval;
  
  wdt_reset();

  currentTime = millis();

  //=== Current
  int Raw = analogRead(CURRENT_PIN);
  if (Raw > RawMax) RawMax = Raw;
  RawCnt++;
  if (RawCnt > 1000){
    RawLast = RawMax;
    RawMax = INT_MIN;
    RawCnt = 0;

    if (LightState == RELAY_ON){
      // Source is low
      if (RawLast < 1){              
        strlcpy(ErrorMsg, "No load", sizeof(ErrorMsg));
        ErrorTry = 5;
        ErrorTime = 0;
      }

      // Owerload
      if (RawLast > CURRENT_MaxLoad*2) {        
        LightState = RELAY_OFF;
        digitalWrite(LIGHT_PIN, LightState);

        strlcpy(ErrorMsg, "Overload", sizeof(ErrorMsg));
        ErrorTry = 200;
        ErrorTime = 0;
      }
    }
  }  

  // Temp overload
  fval = ReadTmp();
  if (fval >= TempMax){
    LightState = RELAY_OFF;
    digitalWrite(LIGHT_PIN, LightState);

    strlcpy(ErrorMsg, "Over temp", sizeof(ErrorMsg));
    ErrorTry = 200;
    ErrorTime = 0;
  }

  //=== Light state to gate
  if ((LightStateSendTry != 0) && (currentTime - LightStateSendTime > SendTryTimeOut)) {
    if (SendData(0, LIGHT_ID, V_LIGHT, LightState?RELAY_ON:RELAY_OFF, true)) {
      LightStateSendTry = 0;      
    }

    LightStateSendTime = currentTime;
  }
  
  //=== Repeater
  // Send feq
  // Only send values at a maximum frequency or woken up from sleep  
  if (currentTime - lastSend > SEND_FREQUENCY){
    sendHeartbeat();

    // Temp
    fval = ReadTmp();
    if (fval != TempLast){
      wait(50);
      SendData(0, TEMP_ID, V_TEMP, fval, 1);
      TempLast = fval;
    }

    // Current
    wait(50);
    SendData(0, CURRENT_ID, V_CURRENT, ReadCurrent(), 3);
      
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

float ReadTmp(){
  int i;
  float media;
  int amostra[TEMP_NUMAMOSTRAS];

  for (i=0; i< TEMP_NUMAMOSTRAS; i++) {
    amostra[i] = analogRead(TEMP_PIN);    
    delay(10);
  }
  
  media = 0;
  for (i=0; i< TEMP_NUMAMOSTRAS; i++) {
    media += amostra[i];
  }
  media /= TEMP_NUMAMOSTRAS;
  // Convert the thermal stress value to resistance
  media = 1023 / media - 1;
  media = TEMP_SERIESRESISTOR / media;
  
  //Calculate temperature using the Beta Factor equation
  float temperatura;

  temperatura = media / TEMP_TERMISTORNOMINAL; // (R/Ro)
  temperatura = log(temperatura); // ln(R/Ro)
  temperatura /= TEMP_BCOEFFICIENT; // 1/B * ln(R/Ro)
  temperatura += 1.0 / (TEMP_TEMPERATURENOMINAL + 273.15); // + (1/To)
  temperatura = 1.0 / temperatura; // Invert the value
  temperatura -= 273.15;    

  #ifdef debug
     Serial.print("Temp RAW:");
     Serial.println(temperatura);
  #endif

  return temperatura;
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

float ReadCurrentVolt(){   
  if (RawLast == INT_MIN)
    return NAN;
  else {
    if (RawLast == INT_MIN) 
      return NAN;
    else
      return (2.5*RawLast) / 1024.0; // Gets you mV  
  }
}

float ReadCurrent(){ 
  float Val = ReadCurrentVolt();
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
       case LIGHT_ID:
           SendData(Dest, LIGHT_ID, V_LIGHT, LightState, false);
           break;
           
       case TEMP_ID:
          switch (message.type) {
            case V_TEMP:
              SendData(Dest, TEMP_ID, V_TEMP, ReadTmp(), 1, false);
              break;
            case V_VAR3:
              SendData(Dest, TEMP_ID, V_TEMP, TempMax, false);
              break;
          }
          break;
           
       case CURRENT_ID:
           switch (message.type) {
            // Vltage
            case V_VOLTAGE:              
              SendData(Dest, CURRENT_ID, V_VOLTAGE, ReadCurrentVolt(), 3, false);
              break;

            // Raw data
            case V_CUSTOM:              
              SendData(Dest, CURRENT_ID, V_CUSTOM, RawLast, false);
              break;

            // Offset
            case V_VAR1:
              SendData(Dest, CURRENT_ID, V_VAR1, CURRENT_Offset, false);
              break;

            // Max load
            case V_VAR3:
              SendData(Dest, CURRENT_ID, V_VAR1, CURRENT_MaxLoad, false);
              break;
                      
            default:
              SendData(Dest, CURRENT_ID, V_CURRENT, ReadCurrent(), 3, false);              
              break;
           }
           
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

     // If set state not from gate
     if (Dest != 0){
       LightStateSendTry = SendTry;
       LightStateSendTime = 0;       
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
  // Current state
  else if (message.sensor == CURRENT_ID){
      switch (message.type) {
        case V_VAR1:
          CURRENT_Offset = int8_t(message.getInt());     
          saveState(CURRENT_ID*10+0, uint8_t(128+CURRENT_Offset));
    
          #ifdef DEBUG
            Serial.print("Set current offset: ");
            Serial.println(CURRENT_Offset);
          #endif  
          break;
          
        case V_VAR3:
          CURRENT_MaxLoad = int8_t(message.getInt());     
          saveState(CURRENT_ID*10+2, uint8_t(CURRENT_MaxLoad));
    
          #ifdef DEBUG
            Serial.print("Set current max load: ");
            Serial.println(CURRENT_MaxLoad);
          #endif  
          break;
      }      
  }
  // Current state
  else if (message.sensor == TEMP_ID){
      switch (message.type) {
        case V_VAR3:
          TempMax = int8_t(message.getInt());     
          saveState(TEMP_ID*10+2, uint8_t(TempMax));
    
          #ifdef DEBUG
            Serial.print("Set temp max: ");
            Serial.println(TempMax);
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

