// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID 0 

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <DHT.h>
#include <EEPROM.h>

#define BATTERY_AS_SENSOR

#define SKETCH_NAME "Binary Sensor+DHT"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

#define CHILD_ID_TRAP 1
#define CHILD_ID_HUM 2
#define CHILD_ID_TEMP 3
#define CHILD_ID_DEV 254

#define cMaxSmallTry 3

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3300 // full voltage (100%)

unsigned long SEND_FREQUENCY = 900000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway.
unsigned long SEND_FREQUENCY_NOTSEND = 8000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway.  

#define TRAP_PIN 2
#define DHT_DATA_PIN 3

#if (TRAP_PIN < 2 || TRAP_PIN > 3)
#error TRAP_PIN must be either 2 or 3 for interrupts to work
#endif 

uint8_t sendValue=2;
long sendtry=0; 

DHT dht;
float lastTemp;
float lastHum;
boolean metric = true; 
//double oldTemp = 0;

float temperature;
float humidity;

long errsend=0;       
long lasterrsend=0; 

MyMessage msgTrap(CHILD_ID_TRAP, V_TRIPPED);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
#ifdef BATTERY_AS_SENSOR
  //MyMessage TempMsg(CHILD_ID_DEV, V_TEMP);
  MyMessage BattMsg(CHILD_ID_DEV, V_VOLTAGE);
#endif  

#ifdef SendSkipTry
  MyMessage ErrSend(CHILD_ID_DEV, V_VAR4); 
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

double GetTemp(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
} 

void SendDevInfo()
{
  //========= Temperature =============
  /*double fTemp = GetTemp();
  
  if (oldTemp != fTemp){
    Serial.print("Temp:");
    Serial.println(fTemp);
  
    send(TempMsg.set(fTemp, 2));
      
    oldTemp = fTemp;
  }
  */

  //========= Battery =============  
  #ifdef BATTERY_AS_SENSOR
    float batteryV  = readVcc() * 0.001; 
    send(BattMsg.set(batteryV, 2));
  
    Serial.print("BatV:");
    Serial.println(batteryV); 
  #else
    int batteryV = readVcc();
    int batteryPcnt = min(map(batteryV, MIN_V, MAX_V, 0, 100), 100);
    sendBatteryLevel( batteryPcnt ); 
    
    Serial.print("BatV:");
    Serial.println(batteryV * 0.001);  
  #endif
  
  #ifdef SendSkipTry
    if (lasterrsend != errsend){
      send(ErrSend.set(errsend));
      lasterrsend = errsend;
    }
  #endif 
}

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
  
  // Setup the buttons
  pinMode(TRAP_PIN, INPUT);
  pinMode(DHT_DATA_PIN, INPUT);
 
  // Activate internal pull-ups
  digitalWrite(TRAP_PIN, HIGH); 

  // DHT
  dht.setup(DHT_DATA_PIN);  
  
  wdt_enable(WDTO_8S);
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);
 
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_TRAP, S_DOOR, "Open sensor");  
  present(CHILD_ID_HUM,  S_HUM,  "Humedy");
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  
  metric = getConfig().isMetric;
}

void ReadDHT()
{  
  delay( dht.getMinimumSamplingPeriod() );

  temperature = dht.getTemperature();
  if (isnan(temperature)) {
    #ifdef MY_DEBUG  
      Serial.println("Failed reading temperature from DHT");
    #endif
  } else if (temperature != lastTemp) {
    if (!metric) temperature = dht.toFahrenheit(temperature);    

    #ifdef MY_DEBUG  
      Serial.print("T: ");
      Serial.println(temperature);
    #endif  
    
    if (!send(msgTemp.set(temperature, 1))){
      // Not sended      
      lastTemp = NAN;
    } else {
      lastTemp = temperature;
    }
  }
  
  humidity = dht.getHumidity();
  if (isnan(humidity)) {
      #ifdef MY_DEBUG  
        Serial.println("Failed reading humidity from DHT");
      #endif
  } else if (humidity != lastHum) {
      #ifdef MY_DEBUG  
        Serial.print("H: ");
        Serial.println(humidity);
      #endif
      
      if (!send(msgHum.set(humidity, 1))){
        // Not sended
        lastHum = NAN;
      } else {
        lastHum = humidity;
      }     
  }
  
//  dht.resetTimer(); // rese reading timer
}

void loop()      
{
  unsigned long SleepTime;

  wdt_reset();
  
  // Short delay to allow buttons to properly settle
  sleep(5); 
  
  // Door sensor
  uint8_t value = digitalRead(TRAP_PIN); 
  if (value != sendValue) {
     // Value has changed from last transmission, send the updated value
     if (!send(msgTrap.set(value==HIGH ? 1 : 0))){
       // Not send last state       
       if (sendtry < cMaxSmallTry){
         sendtry++;
         errsend++;
       }
     } else {
       sendValue = value;
       sendtry = 0;
     }
  } 

  // Sleep until something happens with the sensor
  // Sleep time
  if (
       ((sendtry >= 1) && (sendtry < cMaxSmallTry))
     ){
    SleepTime = SEND_FREQUENCY_NOTSEND;
    Serial.println("sleep small");
  } else {
    SleepTime = SEND_FREQUENCY;
    Serial.println("sleep long");
  }
  
  if ( sleep(TRAP_PIN-2, CHANGE, SleepTime) == -1){
    SendDevInfo();
    ReadDHT();  
  }; 
}
