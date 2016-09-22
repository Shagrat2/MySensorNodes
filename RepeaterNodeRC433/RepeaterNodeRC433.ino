// #define SYS_BAT - Батарею через функцию MySensor
#define MY_NODE_ID 6
#define MY_PARENT_NODE_ID 0

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>
#include <EEPROM.h>
#include <RCSwitch.h>

unsigned long SEND_FREQUENCY = 30000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 
unsigned long RECEIVE_TIMEOUT = 500;

#define RECEIVEPIN 0  // 2 Pin Interupt
#define SENDPID 4

#define TRY_TO_SEND 5

//===============================================
RCSwitch mySwitch = RCSwitch();

unsigned long currentTime; 
unsigned long lastSend;
unsigned long lastRCSend;
unsigned long lastRCId;
unsigned long SendRCId = 0;
unsigned long SendTry = 0;

MyMessage RCMsg(MY_NODE_ID, V_IR_RECEIVE);

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
  mySwitch.enableReceive( RECEIVEPIN );
  mySwitch.enableTransmit( SENDPID );    
  
  wdt_enable(WDTO_8S);  
  
  lastSend=millis();
  lastRCSend=0;
  lastRCId = 0;
}

void presentation()  {
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo("Repeater Node witch RC", "2.0");

  present(MY_NODE_ID, S_ARDUINO_NODE, "RC433");  
}

void loop()
{
  wdt_reset();

  currentTime = millis();

  //==== RC send
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

  if (message.sensor != MY_NODE_ID) return;
  if (message.type != V_IR_SEND) return;

  // Set to task
  SendRCId = message.getLong();
  SendTry = TRY_TO_SEND;
}

