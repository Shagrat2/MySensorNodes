// #define SYS_BAT - Батарею через функцию MySensor
#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>  
#include <avr/wdt.h>
#include <EEPROM.h>

#define DIGITAL_INPUT_SENSOR 2                  // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)
#define PULSE_FACTOR 100                        // Nummber of blinks per m3 of your meter (One rotation/liter)
#define SLEEP_MODE true                         // flowvalue can only be reported when sleep mode is false.
#define MAX_FLOW 40                             // Max flow (l/min) value to report. This filters outliers.
#define SENSOR_INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
unsigned long SEND_FREQUENCY = 900000;          // Minimum time between send (in milliseconds). We don't want to spam the gateway.

double ppl = ((double)PULSE_FACTOR)/1000;        // Pulses per liter

// Id of the sensor child
#define CHILD_ID     1
#define CHILD_ID_DEV 254

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
unsigned long currentTime;

#define NONE_TEMP -9999
double oldTemp = NONE_TEMP;
float oldBatteryV = 0;

float tGain = 0.9338;
float tOffset = 300;

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3700 // full voltage (100%)

MyMessage flowMsg(CHILD_ID,V_FLOW);
MyMessage volumeMsg(CHILD_ID,V_VOLUME);
MyMessage lastCounterMsg(CHILD_ID,V_VAR1);

MyMessage TempMsg(CHILD_ID_DEV, V_TEMP);
#ifndef SYS_BAT
  MyMessage BattMsg(CHILD_ID_DEV, V_VOLTAGE);
#endif 

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

float GetTemp() {
  signed long resultTemp;
  float resultTempFloat;
  // Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(10);                           // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                 // Convert
  while (bit_is_set(ADCSRA,ADSC));
  resultTemp = ADCL;
  resultTemp |= ADCH<<8;
  if (resultTemp == 0){
    resultTempFloat = NONE_TEMP;
  } else {
    resultTempFloat = (float) resultTemp * tGain - tOffset; // Apply calibration correction
  }
  return resultTempFloat;
}

void SendDevInfo()
{
  //========= Temperature =============
  double fTemp = GetTemp();
  
  if ((oldTemp != fTemp) && (fTemp != NONE_TEMP)){
    Serial.print("Temp:");
    Serial.println(fTemp);
  
    if (!send(TempMsg.set(fTemp, 2))){
      oldTemp = NONE_TEMP;
    } else {
      oldTemp = fTemp;
    }
  }    

  int batteryV = readVcc();
  if (batteryV != 0){
    Serial.print("BatV:");
    Serial.println(batteryV * 0.001);        
    
    #ifdef SYS_BAT
      int batteryPcnt = min(map(batteryV, MIN_V, MAX_V, 0, 100), 100);
      sendBatteryLevel( batteryPcnt );
    #else    
      batteryV  = batteryV * 0.001;
      send(BattMsg.set(batteryV, 2));   
    #endif    
  }  
} 

void setup()  
{  
  wdt_disable();

  // Disable the ADC by setting the ADEN bit (bit 7) to zero.
  ADCSRA = ADCSRA & B01111111;
  
  // Disable the analog comparator by setting the ACD bit
  // (bit 7) to one.
  ACSR = B10000000;   

  // Init adress from pin
  if (analogRead(3) >= 1023){
    Serial.begin(115200);
    Serial.println("Init adress");
    
    for (int i=0;i<512;i++) {
      EEPROM.write(i, 0xFF);
    } 
  }

  pulseCount = oldPulseCount = 0;

  lastSend = lastPulse = millis();

  attachInterrupt(SENSOR_INTERRUPT, onPulse, RISING);

  wdt_enable(WDTO_8S);  
  
  lastSend=millis();
}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Natural GAS Meter", "1.2_MD");

  // Register this device as Waterflow sensor
  present(CHILD_ID, S_CUSTOM,   "Pulse sensor");
  present(CHILD_ID_DEV, S_TEMP, "Internal sensor");  
}

void loop()     
{ 
  wdt_reset();
    
  unsigned long currentTime = millis();
	
    // Only send values at a maximum frequency or woken up from sleep
  if (SLEEP_MODE || (currentTime - lastSend > SEND_FREQUENCY))
  {
    lastSend=currentTime;
    
    if (!pcReceived) {
      sleep(8000);
      
      //Last Pulsecount not yet received from controller, request it again
      request(CHILD_ID, V_VAR1);

      wait(500);
      return;
    }

    if (!SLEEP_MODE && flow != oldflow) {
      oldflow = flow;

      Serial.print("l/min:");
      Serial.println(flow);

      // Check that we dont get unresonable large flow value. 
      // could hapen when long wraps or false interrupt triggered
      if (flow<((unsigned long)MAX_FLOW)) 
        send(flowMsg.set(flow, 2)); // Send flow value to gw       
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

      send(lastCounterMsg.set(pulseCount)); // Send  pulsecount value to gw in VAR1

      double volume = ((double)pulseCount/((double)PULSE_FACTOR));     
      if (volume != oldvolume) {
        oldvolume = volume;

        Serial.print("volume:");
        Serial.println(volume, 3);
        
        send(volumeMsg.set(volume, 3)); // Send volume value to gw
      } 
    }
  }
  if (SLEEP_MODE) {
    SendDevInfo();
    
    Serial.println("sleep");
    sleep(SEND_FREQUENCY);
  }
}

void receive(const MyMessage &message) {
  Serial.println("Receive");
  
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
}
