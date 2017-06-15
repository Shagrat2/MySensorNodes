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

#define SKETCH_NAME "Natural GAS Meter"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "3"

#define S2 // Hav S2 sensor

//#define MY_SMART_SLEEP_WAIT_DURATION_MS (200ul)

#define S1_CHILD_ID     1
#define S1_DIGITAL_INPUT_SENSOR 2                     // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)
#define S1_PULSE_FACTOR 100                           // Nummber of blinks per m3 of your meter (One rotation/liter)
#define S1_MAX_FLOW 40                                // Max flow (l/min) value to report. This filters outliers.
#define S1_SENSOR_INTERRUPT S1_DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)

#ifdef S2
  #define S2_CHILD_ID     2
  #define S2_DIGITAL_INPUT_SENSOR 3                     // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)
  #define S2_PULSE_FACTOR 100                           // Nummber of blinks per m3 of your meter (One rotation/liter)
  #define S2_MAX_FLOW 40                                // Max flow (l/min) value to report. This filters outliers.
  #define S2_SENSOR_INTERRUPT S2_DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#endif

#define SLEEP_MODE true                               // flowvalue can only be reported when sleep mode is false.
unsigned long SEND_FREQUENCY = 900000;                // Minimum time between send (in milliseconds). We don't want to spam the gateway.

double S1_ppl = ((double)S1_PULSE_FACTOR)/1000;          // Pulses per liter
#ifdef S2
  double S2_ppl = ((double)S2_PULSE_FACTOR)/1000;          // Pulses per liter
#endif

// Id of the sensor child
#define CHILD_ID_DEV 254

unsigned long lastSend = 0;

volatile unsigned long S1_pulseCount = 0;   
volatile unsigned long S1_lastBlink = 0;
volatile double S1_flow = 0;  
boolean S1_pcReceived = false;

unsigned long S1_oldPulseCount = 0;
unsigned long S1_newBlink = 0;   
double S1_oldflow = 0;
double S1_volume = 0;
double S1_oldvolume = 0;
unsigned long S1_lastPulse =0;

#ifdef S2
  volatile unsigned long S2_pulseCount = 0;   
  volatile unsigned long S2_lastBlink = 0;
  volatile double S2_flow = 0;  
  boolean S2_pcReceived = false;
  
  unsigned long S2_oldPulseCount = 0;
  unsigned long S2_newBlink = 0;   
  double S2_oldflow = 0;
  double S2_volume = 0;
  double S2_oldvolume = 0;
  unsigned long S2_lastPulse =0;
#endif

unsigned long currentTime;

#define NONE_TEMP -9999
double oldTemp = NONE_TEMP;
float oldBatteryV = 0;

float tGain = 0.9338;
float tOffset = 300;

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3700 // full voltage (100%)

MyMessage S1_flowMsg(S1_CHILD_ID,V_FLOW);
MyMessage S1_volumeMsg(S1_CHILD_ID,V_VOLUME);
MyMessage S1_lastCounterMsg(S1_CHILD_ID,V_VAR1);

#ifdef S2
  MyMessage S2_flowMsg(S2_CHILD_ID,V_FLOW);
  MyMessage S2_volumeMsg(S2_CHILD_ID,V_VOLUME);
  MyMessage S2_lastCounterMsg(S2_CHILD_ID,V_VAR1);
#endif

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
      send(BattMsg.set(batteryV*0.001, 2));   
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

  lastSend = S1_lastPulse = millis();

  S1_pulseCount = S1_oldPulseCount = 0;  

  #ifdef S2
    S2_pulseCount = S2_oldPulseCount = 0;    
  #endif

  attachInterrupt(S1_SENSOR_INTERRUPT, onPulseS1, RISING);
  #ifdef S2
    attachInterrupt(S2_SENSOR_INTERRUPT, onPulseS2, RISING);
  #endif

  wdt_enable(WDTO_8S);  
  
  lastSend=millis();
}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  // Register this device as Waterflow sensor
  present(S1_CHILD_ID, S_CUSTOM, "Pulse sensor S1");
  wait(50);
  #ifdef S2
    present(S2_CHILD_ID, S_CUSTOM, "Pulse sensor S2");
    wait(50);
  #endif
  present(CHILD_ID_DEV, S_TEMP, "Internal sensor");  
}

void loop()     
{ 
  wdt_reset();
    
  unsigned long currentTime = millis();
	
  // Only send values at a maximum frequency or woken up from sleep
  if (SLEEP_MODE || (currentTime - lastSend > SEND_FREQUENCY))
  {
    lastSend = currentTime;

    #ifndef S2
      if (!S1_pcReceived)
    #else
      if ((!S1_pcReceived) || (!S2_pcReceived)) 
    #endif
    {
      smartSleep(8000);
      
      // Last Pulsecount not yet received from controller, request it again
      if (!S1_pcReceived){
        request(S1_CHILD_ID, V_VAR1);
        wait(500);
      }

      #ifdef S2
        if (!S2_pcReceived){
          request(S2_CHILD_ID, V_VAR1);
          wait(500);
        }
      #endif

      return;
    }

    if (!SLEEP_MODE && S1_flow != S1_oldflow) {
      S1_oldflow = S1_flow;

      Serial.print("S1 l/min:");
      Serial.println(S1_flow);

      // Check that we dont get unresonable large flow value. 
      // could hapen when long wraps or false interrupt triggered
      if (S1_flow<((unsigned long)S1_MAX_FLOW)) 
        send(S1_flowMsg.set(S1_flow, 2)); // Send flow value to gw       
    }

    #ifdef S2
      if (!SLEEP_MODE && S2_flow != S2_oldflow) {
        S2_oldflow = S2_flow;
  
        Serial.print("S2 l/min:");
        Serial.println(S2_flow);
  
        // Check that we dont get unresonable large flow value. 
        // could hapen when long wraps or false interrupt triggered
        if (S2_flow<((unsigned long)S2_MAX_FLOW)) 
          send(S2_flowMsg.set(S2_flow, 2)); // Send flow value to gw       
      }
    #endif
  
    // No Pulse count received in 2min 
    if(currentTime - S1_lastPulse > 120000){
      S1_flow = 0;
    } 

    // No Pulse count received in 2min 
    #ifdef S2
      if(currentTime - S2_lastPulse > 120000){
        S2_flow = 0;
      } 
    #endif

    // Pulse count has changed
    if (S1_pulseCount != S1_oldPulseCount) {
      S1_oldPulseCount = S1_pulseCount;

      Serial.print("pulsecount S1:");
      Serial.println(S1_pulseCount);

      send(S1_lastCounterMsg.set(S1_pulseCount)); // Send  pulsecount value to gw in VAR1

      double S1_volume = ((double)S1_pulseCount/((double)S1_PULSE_FACTOR));     
      if (S1_volume != S1_oldvolume) {
        S1_oldvolume = S1_volume;

        Serial.print("volume:");
        Serial.println(S1_volume, 3);
        
        send(S1_volumeMsg.set(S1_volume, 3)); // Send volume value to gw
      } 
    }

    // Pulse count has changed
    #ifdef S2
      if (S2_pulseCount != S2_oldPulseCount) {
        S2_oldPulseCount = S2_pulseCount;
  
        Serial.print("pulsecount S2:");
        Serial.println(S2_pulseCount);
  
        send(S2_lastCounterMsg.set(S2_pulseCount)); // Send  pulsecount value to gw in VAR1
  
        double S2_volume = ((double)S2_pulseCount/((double)S2_PULSE_FACTOR));     
        if (S2_volume != S2_oldvolume) {
          S2_oldvolume = S2_volume;
  
          Serial.print("volume:");
          Serial.println(S2_volume, 3);
          
          send(S2_volumeMsg.set(S2_volume, 3)); // Send volume value to gw
        } 
      }
    #endif
  }
  if (SLEEP_MODE) {
    SendDevInfo();
    
    Serial.println("sleep");
    smartSleep(SEND_FREQUENCY);
  }
}

void receive(const MyMessage &message) {
  Serial.println("Receive");

  unsigned long gwPulseCount;

  if ((message.sensor == S1_CHILD_ID) && (message.type == V_VAR1)) {  
    gwPulseCount = message.getULong();
    S1_pulseCount += gwPulseCount;
    Serial.print("Received last pulse count from gw S1:");
    Serial.println( S1_pulseCount );
    S1_pcReceived = true;
  }

  #ifdef S2
    if ((message.sensor == S2_CHILD_ID) && (message.type == V_VAR1)) {  
      gwPulseCount = message.getULong();
      S2_pulseCount += gwPulseCount;
      Serial.print("Received last pulse count from gw S2:");
      Serial.println( S2_pulseCount );
      S2_pcReceived = true;
    }
  #endif
}

void onPulseS1()
{
  if (!SLEEP_MODE)
  {
    unsigned long newBlink = micros();   
    unsigned long S1_interval = newBlink-S1_lastBlink;
    
    if (S1_interval!=0)
    {
      S1_lastPulse = millis();
      if (S1_interval<500000L) {
        // Sometimes we get interrupt on RISING,  500000 = 0.5sek debounce ( max 120 l/min)
        return;   
      }
      S1_flow = (60000000.0 /S1_interval) / S1_ppl;
    }
    S1_lastBlink = newBlink;
  }
  S1_pulseCount++; 
}

#ifdef S2
void onPulseS2()
{
  if (!SLEEP_MODE)
  {
    unsigned long newBlink = micros();   
    unsigned long S2_interval = newBlink-S2_lastBlink;
    
    if (S2_interval!=0)
    {
      S2_lastPulse = millis();
      if (S2_interval<500000L) {
        // Sometimes we get interrupt on RISING,  500000 = 0.5sek debounce ( max 120 l/min)
        return;   
      }
      S2_flow = (60000000.0 /S2_interval) / S2_ppl;
    }
    S2_lastBlink = newBlink;
  }
  S2_pulseCount++; 
}
#endif
