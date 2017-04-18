//
// Use this sensor to measure volume and flow of your house watermeter.
// You need to set the correct pulsefactor of your meter (pulses per m3).
// The sensor starts by fetching current volume reading from gateway (VAR 1).
// Reports both volume and flow back to gateway.
//
// Unfortunately millis() won't increment when the Arduino is in 
// sleepmode. So we cannot make this sensor sleep if we also want  
// to calculate/report flow.
//

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#define DIGITAL_INPUT_SENSOR 3                  // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)
#define SENSOR_INTERRUPT DIGITAL_INPUT_SENSOR-2        // Usually the interrupt = pin -2 (on uno/nano anyway)

#define PULSE_FACTOR 450                        // Nummber of blinks per m3 of your meter (One rotation/liter)

#define SLEEP_MODE false                        // flowvalue can only be reported when sleep mode is false.

#define MAX_FLOW 40                             // Max flow (l/min) value to report. This filters outliers.

#define CHILD_ID 1                              // Id of the sensor child
#define CHILD_ID_DEV 254

unsigned long SEND_FREQUENCY = 30000;           // Minimum time between send (in milliseconds). We don't want to spam the gateway.

MyMessage flowMsg(CHILD_ID,V_FLOW);
MyMessage volumeMsg(CHILD_ID,V_VOLUME);
MyMessage lastCounterMsg(CHILD_ID,V_VAR1);

double ppl = ((double)PULSE_FACTOR)/1000;        // Pulses per liter

MyMessage PongMsg(CHILD_ID_DEV, V_SCENE_ON);

volatile unsigned long pulseCount = 0;   
volatile unsigned long lastBlink = 0;
volatile double flow = 0;  
boolean pcReceived = false;
unsigned long oldPulseCount = 0;
unsigned long newBlink = 0;   
double oldflow = 0;
double volume =0;                     
double oldvolume =0;
unsigned long lastSend =0;
unsigned long lastPulse =0;

void setup()  
{  
  wdt_disable();
  
  // Init adress from pin
  if (analogRead(3) >= 1023){
    Serial.begin(115200);
    Serial.println("Init adress");
    
    for (int i=0;i<512;i++) {
      EEPROM.write(i, 0xff);
    } 
  } 
  
  // Disable the ADC by setting the ADEN bit (bit 7) to zero.
  ADCSRA = ADCSRA & B01111111;
  
  // Disable the analog comparator by setting the ACD bit
  // (bit 7) to one.
  ACSR = B10000000;  

  // === Get last count
  pulseCount = oldPulseCount = 0;

  // Fetch last known pulse count value from gw
  request(CHILD_ID, V_VAR1);

  lastSend = lastPulse = millis();

  attachInterrupt(SENSOR_INTERRUPT, onPulse, RISING);
  
  wdt_enable(WDTO_8S);
}

void presentation(){ 
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Water Meter", "1.2");

  // Register this device as Waterflow sensor
  present(CHILD_ID, S_WATER);       
}

void loop()     
{ 
  wdt_reset();
  
  unsigned long currentTime = millis();
	
    // Only send values at a maximum frequency or woken up from sleep
  if (SLEEP_MODE || (currentTime - lastSend > SEND_FREQUENCY))
  {
    lastSend=currentTime;
    
    send(PongMsg.set("PONG"));
        
    if (!pcReceived) {
      //Last Pulsecount not yet received from controller, request it again
      request(CHILD_ID, V_VAR1);
      return;
    }

    if (!SLEEP_MODE && flow != oldflow) {
      oldflow = flow;

      Serial.print("l/min:");
      Serial.println(flow);

      // Check that we dont get unresonable large flow value. 
      // could hapen when long wraps or false interrupt triggered
      if (flow<((unsigned long)MAX_FLOW)) {
        send(flowMsg.set(flow, 2));                   // Send flow value to gw
      }  
    }
  
    // No Pulse count received in 2min 
    if(currentTime - lastPulse > 120000){
      flow = 0;
    } 

    // Pulse count has changed
    if (pulseCount != oldPulseCount) {
      oldPulseCount = pulseCount;

      Serial.print("pulsecount:");
      Serial.println(pulseCount);

      send(lastCounterMsg.set(pulseCount));                  // Send  pulsecount value to gw in VAR1

      double volume = ((double)pulseCount/((double)PULSE_FACTOR));     
      if (volume != oldvolume) {
        oldvolume = volume;

        Serial.print("volume:");
        Serial.println(volume, 3);
        
        send(volumeMsg.set(volume, 3));               // Send volume value to gw
      } 
    }
  }
  if (SLEEP_MODE) {
    sleep(SEND_FREQUENCY);
  }
}

void receive(const MyMessage &message) {
  if (message.type==V_VAR1) {
    unsigned long gwPulseCount=message.getULong();
    pulseCount += gwPulseCount;
    Serial.print("Received last pulse count from gw:");
    Serial.println(pulseCount);
    pcReceived = true;
  }
}

void onPulse()     
{
  if (!SLEEP_MODE)
  {
    unsigned long newBlink = micros();   
    unsigned long interval = newBlink-lastBlink;
    
    if (interval!=0)
    {
      lastPulse = millis();
      if (interval<500000L) {
        // Sometimes we get interrupt on RISING,  500000 = 0.5sek debounce ( max 120 l/min)
        return;   
      }
      flow = (60000000.0 /interval) / ppl;
    }
    lastBlink = newBlink;
  }
  pulseCount++; 
  
  Serial.print("Pulse:");
  Serial.println(pulseCount);
}

