// Enable debug prints to serial monitor
#define MY_DEBUG  

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO 

// Example sketch showing how to send in OneWire temperature readings
#include <SPI.h>
#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#define SKETCH_NAME "Dalas Temperatures"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

//#define SYS_BAT
#define CHILD_ID_DEV 254

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3700 // full voltage (100%)

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME = 600000;//15*60*1000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
boolean receivedConfig = false;
boolean metric = true; 
// Initialize temperature message
MyMessage msg(0,V_TEMP);
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

void SendDevInfo()
{
  //========= Battery =============  
  #ifndef SYS_BAT
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
      sensor_node.send(ErrSend.set(errsend));
      lasterrsend = errsend;
    }
  #endif
}  

void before()  
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

  // Startup OneWire 
  sensors.begin();

  wdt_enable(WDTO_8S);
}

void setup(){
}

void presentation()  
{ 
      
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Temperature Sensor", "1.0");
  
  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();

  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i, S_TEMP);
  }  
}

void loop()     
{     
  wdt_reset();

  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures(); 

  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
    wdt_reset();
 
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
 
    // Only send data if temperature has changed and no error
    if (lastTemperature[i] != temperature && temperature != -127.00) {
 
      // Send in the new temperature
      send(msg.setSensor(i).set(temperature,1));
      lastTemperature[i]=temperature;
      
      Serial.print("Sens");
      Serial.print(i);
      Serial.print(":");
      Serial.println(temperature);
    }
  }

  // Device Info
  SendDevInfo();
       
  smartSleep(SLEEP_TIME);
}
