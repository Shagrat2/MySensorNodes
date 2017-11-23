#define MY_PARENT_NODE_ID AUTO
#define MY_REPEATER_FEATURE // Enable repeater functionality for this node

// Flash options
#define MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FLASH_SS 7
//#DEFINE M25P40 // Flash type

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
#include <Bounce2.h>

#ifdef M25P40
	#define SPIFLASH_BLOCKERASE_32K   0xD8
#endif

#define RELAY_PIN  3 // Arduino Digital I/O pin number for first relay
#define RELAY_ID   1 // Relay sensor
#define RELAY_ON   1 // GPIO value to write to turn on attached relay
#define RELAY_OFF  0 // GPIO value to write to turn off attached relay

#define MOTION_ID  5 // Sensor ID

#define MOTION1_PIN 5 // Motion 1
#define MOTION2_PIN 6 // Motion 2

#define LIGHT_PIN A4 // Light sensor
#define LIGHT_ID  7  // Sensor ID

#define TEMP_PIN   A3 // Termistor pin
#define TEMP_ID    2  // Sensor temp ID

#define TEMP_Average  5    // Number of samples to average temperature 
/*
#define TEMP_TERMISTORNOMINAL    10000 // Сопротивление при 25 градусах по Цельсию
#define TEMP_TEMPERATURENOMINAL  25    // temp. для номинального сопротивления (практически всегда равна 25 C)
#define TEMP_NUMAMOSTRAS         5     // сколько показаний используем для определения среднего значения 
#define TEMP_BCOEFFICIENT        3375  // бета коэффициент термистора (обычно 3000-4000)
#define TEMP_SERIESRESISTOR      10000
*/

#define CURRENT_PIN A6      // ACS712 pin
#define CURRENT_ID  3       // Sensor current ID
#define CURRENT_VperAmp 185 // 185=5A, 100=20A, 66=30A

#define RELAYMODE_CELL 20 // State in eprom relay mode

#define RELAY_TIME_H 25 // State of time Sec (High)
#define RELAY_TIME_L 26 // State of time Sec (Low)

#define NODESYSTEM_ID 254

unsigned long SEND_FREQUENCY = 900000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 

unsigned long currentTime;
unsigned long lastSend;
uint8_t  RelayMode = 2; // Mode: 0 - Always OFF, 1 - Always ON, 2 - Auto
uint16_t RelayTime = (2*60); // Time (Sec) ON relay
uint8_t RelayState = 0;
uint8_t RelayStateSendTry = 0;
unsigned long RelayStateSendTime = 0;

int LightLevel = 0; // Level of light
int MinLightLevel = 0; // Minimum of light level

uint8_t CURRENT_MaxLoad = 255;

#define SendTry 8
#define SendTryTimeOut 1000

#define INT_MIN -32767
#define INT_MAX 32767

#define RMODE_OFF   0
#define RMODE_ON    1
#define RMODE_AUTO  2

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

unsigned long LastMotion = 0;
unsigned long LastInvertCode = 0;

Bounce bMotion1 = Bounce(); 
Bounce bMotion2 = Bounce(); 

// Before init MySensors
void before(){
  uint8_t val;
  
  wdt_disable();

  Serial.begin(115200);

  testMode();

  //== Relay
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);

  //== Motion  
  pinMode(MOTION1_PIN, INPUT);   
  pinMode(MOTION2_PIN, INPUT);   
  
  //== Light
  pinMode(LIGHT_PIN, INPUT);

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

  // Relay last mode
  val = loadState(RELAYMODE_CELL);
  if (val != 0xFF){
    ChangeMode(val, 0);
  }

  // Relay last time
  uint8_t valh = loadState(RELAY_TIME_H);
  uint8_t vall = loadState(RELAY_TIME_L);
  uint16_t vald = valh << 8 | vall;
  if (vald != 0xFFFF){
    RelayTime = vald;

    #ifdef DEBUG
      Serial.print("Relay time (sec): ");
      Serial.println(RelayTime);
    #endif
  }

  // Make sure that ATSHA204 is not floating
  //@ set in system

  #ifdef MY_OTA_FIRMWARE_FEATURE  
    #ifdef DEBUG
      Serial.println("OTA FW update enabled");
    #endif
  #endif

  wdt_enable(WDTO_8S);    

  lastSend=millis();
}

void setup() { 
  Serial.println("######## Regist OK");
  
  bMotion1.attach(MOTION1_PIN);
  bMotion1.interval(5); // interval in ms

  bMotion2.attach(MOTION2_PIN);
  bMotion2.interval(5); // interval in ms
}

void presentation()  
{   
  // Node info
  sendSketchInfo("Gate light x2", "1.0.0");

  // Relay
  present(RELAY_ID, S_LIGHT, "Light relay");
  present(RELAY_ID, S_CUSTOM, "Relay mode");

  // Motion
  present(MOTION_ID, S_MOTION, "Motion");

  // Light
  present(LIGHT_ID, S_LIGHT_LEVEL, "Light level");

  // Temp
  present(TEMP_ID, S_TEMP, "Key temperature");

  // Current
  present(CURRENT_ID, S_MULTIMETER, "Source current");
}

void loop() 
{  
  float fval;
  int val;
  
  wdt_reset();

  currentTime = millis();

  //=== Timer off
  if (RelayMode == RMODE_AUTO && RelayState == RELAY_ON) {
    if (currentTime - LastMotion > RelayTime*1000) {
      DoRelay( RELAY_OFF );
    }
  }

  //=== Motion 1  
  bMotion1.update();
  val = bMotion1.read();
  if (val == HIGH) {
    DoMotion(0);
  }

  bMotion2.update();
  val = bMotion2.read();
  if (val == HIGH) {
    DoMotion(1);
  }

  //=== Light
  LightLevel = analogRead(LIGHT_PIN);

  //=== Current
  int Raw = analogRead(CURRENT_PIN);
  if (Raw > RawMax) RawMax = Raw;
  RawCnt++;
  if (RawCnt > 1000){
    RawLast = RawMax;
    RawMax = INT_MIN;
    RawCnt = 0;

    if (RelayState == RELAY_ON){
      // Source is low
      if (RawLast < 1){              
        strlcpy(ErrorMsg, "No load", sizeof(ErrorMsg));
        ErrorTry = 5;
        ErrorTime = 0;
      }
/*
      // Owerload
      if (RawLast > CURRENT_MaxLoad*2) {        
        DoRelay( RELAY_OFF );
        
        strlcpy(ErrorMsg, "Overload", sizeof(ErrorMsg));
        ErrorTry = 200;
        ErrorTime = 0;
      }
*/      
    }
  }  

  // Temp overload
  fval = ReadTmp();
  if (fval >= TempMax){
    DoRelay( RELAY_OFF );
    
    strlcpy(ErrorMsg, "Over temp", sizeof(ErrorMsg));
    ErrorTry = 200;
    ErrorTime = 0;
  }

  //=== Relay state to gate
  if ((RelayStateSendTry != 0) && (currentTime - RelayStateSendTime > SendTryTimeOut)) {
    if (SendData(0, RELAY_ID, V_LIGHT, RelayState?RELAY_ON:RELAY_OFF, true)) {
      RelayStateSendTry = 0;      
    }

    RelayStateSendTime = currentTime;
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

    // Light
    SendData(0, LIGHT_ID, S_LIGHT_LEVEL, LightLevel, false);
      
    lastSend=currentTime;
  }

  //=== Send error
  if (ErrorTry != 0) {
    if (currentTime - ErrorTime > SendTryTimeOut){
      if (SendData(0, NODESYSTEM_ID, V_TEXT, ErrorMsg, true)){
        ErrorTry = 0;
      } else {
        ErrorTry--;
        ErrorTime = currentTime;
      }
    }
  }
}

float ReadTmp(){
  int i;
  float Temp_ADC;
  int amostra[TEMP_Average];

  for (i=0; i< TEMP_Average; i++) {
    amostra[i] = analogRead(TEMP_PIN);    
    delay(10);
  }
  
  Temp_ADC = 0;
  for (i=0; i< TEMP_Average; i++) {
    Temp_ADC += amostra[i];
  }
  Temp_ADC /= TEMP_Average;
  
  //Calculate temperature using the Beta Factor equation
  float temperatura;

  temperatura = -36.9*log(Temp_ADC)+271.4;
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
       case RELAY_ID:
           switch (message.type) {
              case V_LIGHT:
                SendData(Dest, RELAY_ID, V_LIGHT, RelayState, false);
                #ifdef DEBUG
                  Serial.println("# Req RelayState");
                #endif
                break;
                
              case V_VAR1:
                SendData(Dest, RELAY_ID, V_VAR1, RelayMode, false);
                #ifdef DEBUG
                  Serial.println("# Req RelayMode");
                #endif
                break;

              case V_VAR2:
                SendData(Dest, RELAY_ID, V_VAR2, RelayTime, false);
                #ifdef DEBUG
                  Serial.println("# Req RelayTime");
                #endif
                break;
           }
           break;
           
       case TEMP_ID:
          switch (message.type) {
            case V_TEMP:
              SendData(Dest, TEMP_ID, V_TEMP, ReadTmp(), 1, false);
              #ifdef DEBUG
                Serial.println("# Req temp");
              #endif
              break;
            case V_VAR3:
              SendData(Dest, TEMP_ID, V_VAR3, TempMax, false);
              #ifdef DEBUG
                Serial.println("# Req Temp Max");
              #endif
              break;
          }
          break;
           
       case CURRENT_ID:
           switch (message.type) {
            // Vltage
            case V_VOLTAGE:              
              SendData(Dest, CURRENT_ID, V_VOLTAGE, ReadCurrentVolt(), 3, false);
              #ifdef DEBUG
                Serial.println("# Req current as voltage");
              #endif
              break;

            // Raw data
            case V_CUSTOM:              
              SendData(Dest, CURRENT_ID, V_CUSTOM, RawLast, false);
              #ifdef DEBUG
                Serial.println("# Req raw");
              #endif
              break;

            // Offset
            case V_VAR1:
              SendData(Dest, CURRENT_ID, V_VAR1, CURRENT_Offset, false);
              #ifdef DEBUG
                Serial.println("# Req current offset");
              #endif
              break;

            // Max load
            case V_VAR3:
              SendData(Dest, CURRENT_ID, V_VAR3, CURRENT_MaxLoad, false);
              #ifdef DEBUG
                Serial.println("# Req max load");
              #endif
              break;
                      
            default:
              SendData(Dest, CURRENT_ID, V_CURRENT, ReadCurrent(), 3, false);
              #ifdef DEBUG
                Serial.println("# Req current");
              #endif        
              break;
           }     
           break;

      case LIGHT_ID:
        switch (message.type) {
        case V_LIGHT_LEVEL:
          SendData(Dest, LIGHT_ID, V_LIGHT_LEVEL, LightLevel, false);
          #ifdef DEBUG
            Serial.println("# Req LightLevel");
          #endif        
          break;

        case V_VAR1:
          SendData(Dest, LIGHT_ID, V_VAR1, MinLightLevel, false);
          #ifdef DEBUG
            Serial.println("# Req MinLightLevel");
          #endif        
          break;
        }
        break;
    }

    return ;
  }
  
  // Relay state
  if (message.sensor == RELAY_ID) {
    switch (message.type) {
      // Set relay
      case V_STATUS:
        if (message.getBool()){
          ChangeMode(RMODE_ON, Dest);
        } else {
          ChangeMode(RMODE_OFF, Dest);
        }

        #ifdef DEBUG
          Serial.print("# Change relay: ");
          Serial.println(message.getBool());
        #endif
        break;

      // Motion
      case V_TRIPPED:
        DoMotion(255);
        #ifdef DEBUG
          Serial.print("# Motion outside");
        #endif
        break;
      
      // Invert relay
      case V_CUSTOM:
        if (LastInvertCode == message.getInt()) {
          break;
        }

        LastInvertCode = message.getInt();

        switch (RelayMode) {
        case RMODE_ON:
            ChangeMode(RMODE_OFF, Dest);
            break;
        default:
            ChangeMode(RMODE_ON, Dest);
            break;
        }
       
        break;
    
      case V_VAR1:
        ChangeMode(int8_t(message.getInt()), Dest); 
        break;
        
      case V_VAR2:
        RelayTime = uint16_t(message.getInt());

        #ifdef DEBUG
          Serial.print("# Set RelayTime: ");
          Serial.println(RelayTime);
        #endif

        saveState(RELAY_TIME_H, uint8_t(RelayTime >> 8));
        saveState(RELAY_TIME_L, uint8_t(RelayTime));
        break;
    }
  } 
  // Current state
  else if (message.sensor == CURRENT_ID){
      switch (message.type) {
        case V_VAR1:
          CURRENT_Offset = int8_t(message.getInt());
          saveState(CURRENT_ID*10+0, uint8_t(128+CURRENT_Offset));
    
          #ifdef DEBUG
            Serial.print("# Set current offset: ");
            Serial.println(CURRENT_Offset);
          #endif  
          break;
          
        case V_VAR3:
          CURRENT_MaxLoad = int8_t(message.getInt());     
          saveState(CURRENT_ID*10+2, uint8_t(CURRENT_MaxLoad));
    
          #ifdef DEBUG
            Serial.print("# Set current max load: ");
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
            Serial.print("# Set temp max: ");
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

void DoMotion(uint8_t val){
  // Reset timer
  LastMotion = millis();
  
  if (RelayMode == RMODE_AUTO && RelayState == RELAY_OFF) {
    #ifdef DEBUG
      Serial.print("# Do motion: ");
      Serial.println(val);
    #endif
    
    Serial.println("# Motion");
    if (LightLevel > MinLightLevel) {
      DoRelay(RELAY_ON);
    }

    SendData(0, MOTION_ID, V_TRIPPED, val, false); // TODO: Send tray
  }
}

void DoRelay(uint8_t val){
  RelayState = val;
  digitalWrite(RELAY_PIN, RelayState);

  //@ No nide send state - Send when relay mode
}

void ChangeMode(uint8_t mode, uint8_t From){
  RelayMode = mode;

  #ifdef DEBUG
     Serial.print("# Set RelayMode: ");
     Serial.println(RelayMode);
  #endif
        
  switch (RelayMode) {
  // OFF
  case 0:
    DoRelay( RELAY_OFF );
    break;
          
  // ON
  case 1:
    DoRelay( RELAY_ON );
    break;
  }
  saveState(RELAYMODE_CELL, uint8_t(RelayMode));

  // If set state not from gate
  if (From != 0){
    RelayStateSendTry = SendTry;
    RelayStateSendTime = 0;       
  }
}

