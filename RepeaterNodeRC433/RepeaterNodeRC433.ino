#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO
//#define MY_PARENT_NODE_IS_STATIC

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#define HASRCSWITCH

#include <SPI.h>
#include <MySensors.h>
#include <EEPROM.h>

#ifdef HASRCSWITCH
#include <RCSwitch.h>
#endif

#ifdef HASRCSWITCH  
  #define SKETCH_NAME "Repeater Node witch RC"
#else
  #define SKETCH_NAME "Repeater Node"
#endif
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "3"

unsigned long SEND_FREQUENCY = (60*1000ul); //(15*60*1000ul); // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 
unsigned long RECEIVE_TIMEOUT = 500;

#define RC_ID 1

#define RECEIVEPIN 0  // 2 Pin Interupt
#define SENDPID 4

#define TRY_TO_SEND 5

//===============================================
#ifdef HASRCSWITCH
RCSwitch mySwitch = RCSwitch();
#endif

unsigned long currentTime; 
unsigned long lastSend;
unsigned long lastRCSend;
unsigned long lastRCId;
unsigned long SendRCId = 0;
unsigned long SendTry = 0;

#ifdef HASRCSWITCH
  MyMessage RCMsg(RC_ID, V_IR_RECEIVE);
#endif

void setup()  
{  
  wdt_disable();
  
  // Init adress from pin
  if (analogRead(3) >= 1023){
    Serial.begin(115200);    
    Serial.println("Init adress");
    
    for (int i=0;i<512;i++)
      EEPROM.write(i, 0xFF);    
  }  
  
  //=== RC
#ifdef HASRCSWITCH  
  mySwitch.enableReceive( RECEIVEPIN );
  mySwitch.enableTransmit( SENDPID );    
#endif
  
  wdt_enable(WDTO_8S);  
  
  lastSend=millis();
  lastRCSend=0;
  lastRCId = 0;
}

void presentation()  {
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);  

  #ifdef HASRCSWITCH
    
    present(RC_ID, S_ARDUINO_NODE, "RC433");
  #endif
}

void loop()
{
  wdt_reset();

  currentTime = millis();

  //==== RC send
#ifdef HASRCSWITCH  
  if (SendTry != 0)
  {
    Serial.print("Send RC ");
    Serial.print(SendTry);
    Serial.print(":");
    Serial.println(SendRCId);  

    mySwitch.send( SendRCId, 24 );
    SendTry--;
  } 

  //==== RC receive
  if (mySwitch.available()) {    
    Serial.print("Receive ");
    Serial.print(mySwitch.getReceivedValue());
    Serial.print("-");
    Serial.print(mySwitch.getReceivedBitlength());
    Serial.print("bit-P");
    Serial.println(mySwitch.getReceivedProtocol());    

    if ((mySwitch.getReceivedBitlength() == 24) && (mySwitch.getReceivedProtocol() == 1))
    {
      unsigned long id = mySwitch.getReceivedValue();
      if ((currentTime - lastRCSend > RECEIVE_TIMEOUT) || (lastRCId != id))
        if (send(RCMsg.set(id), true))
        {
          lastRCSend = currentTime;        
          lastRCId = id;
        }
    }
    
    mySwitch.resetAvailable();
  }  
#endif  

  //=== Repeater
  // Send feq
  // Only send values at a maximum frequency or woken up from sleep  
  if (currentTime - lastSend > SEND_FREQUENCY)
  {
    sendHeartbeat();
    lastSend=currentTime;
  }
}

void receive(const MyMessage &message) 
{ 
  if (message.isAck()) return;

  if (message.sensor != RC_ID) return;
  if (message.type != V_IR_SEND) return;

  // Set to task
  SendRCId = message.getLong();
  SendTry = TRY_TO_SEND;
}

