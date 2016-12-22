// Enable debug prints to serial monitor
#define MY_DEBUG  

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <DHT.h>
#include <EEPROM.h>

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO 

#define BATTERY_AS_SENSOR

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_DEV 254

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3300 // full voltage (100%)

#define DHTPIN 2

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

unsigned long SLEEP_TIME = 300000; // Sleep time between reads (in milliseconds)

DHT dht(DHTPIN, DHTTYPE);

float lastTemp;
float lastHum;
boolean metric = true; 
//double oldTemp = 0;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
#ifdef BATTERY_AS_SENSOR
  //MyMessage TempMsg(CHILD_ID_DEV, V_TEMP);
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
  
  dht.begin();
  
  wdt_enable(WDTO_8S);
}

void presentation() {
    // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM,  S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  
  metric = getConfig().isMetric;
}

void loop()      
{  
  wdt_reset();

  float temperature = temperature = dht.readTemperature(!metric);    
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {    
    Serial.print("T: ");
    Serial.println(temperature);
    
    if (!send(msgTemp.set(temperature, 1))){
      // Not sended      
      lastTemp = NAN;
    } else {
      lastTemp = temperature;
    }
  }
  
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {      
      Serial.print("H: ");
      Serial.println(humidity);
      
      if (!send(msgHum.set(humidity, 1))){
        // Not sended
        lastHum = NAN;
      } else {
        lastHum = humidity;
      }     
  }
  
  SendDevInfo();

  sleep(SLEEP_TIME); //sleep a bit
}
