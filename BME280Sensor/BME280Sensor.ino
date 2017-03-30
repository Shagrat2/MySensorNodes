// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

//#define IsMQTT

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

#define SLEEP_TIME 300000 // Sleep time between reads (in milliseconds) 

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <EEPROM.h> 

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 2
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SKETCH_NAME "BME280 Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

Adafruit_BME280 bme; // I2C

#define HUM_ID 3
#define PRESS_ID 4
#define TEMP_ID 4
#define CHILD_ID_DEV 254

MyMessage msgHum(HUM_ID, V_HUM);
MyMessage msgPress(PRESS_ID, V_PRESSURE);
MyMessage msgTemp(TEMP_ID, V_TEMP);

MyMessage TempMsg(CHILD_ID_DEV, V_TEMP);
#ifdef IsMQTT
  MyMessage BattMsg(CHILD_ID_DEV, V_VOLTAGE);
#endif

float LastTemp = 0;
float LastPress = 0;
float LastHum = 0;

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3700 // full voltage (100%)

float tGain = 0.9338;
float tOffset = 290.7;

double oldTemp = 0;

void setup()  
{  
  wdt_disable(); 

  Serial.begin(115200);
  
  // Init adress from pin
  if (analogRead(3) >= 1023){    
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

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  wdt_enable(WDTO_8S);  
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  present(TEMP_ID, S_TEMP, "Temperature sensor");
  present(HUM_ID, S_HUM, "Humidity sensor");
  present(PRESS_ID, S_BARO, "Pressure sensor");
} 

// Loop will iterate on changes on the BUTTON_PINs
void loop() 
{
  wdt_reset();

  float Temp = bme.readTemperature();
  float Press = bme.readPressure() * 0.750062; // мм рт ст
  float Hum = bme.readHumidity();

  if (LastTemp != Temp)
    if (send(msgTemp.set(Temp, 1)))
      LastTemp = Temp;
  
  if (LastPress != Press)
    if (send(msgPress.set(Press, 1)))
      LastPress = Press; 

  if (LastHum != Hum)
    if (send(msgHum.set(Hum, 1)))
      LastHum = Hum;       

  sendHeartbeat();

  sleep( SLEEP_TIME );

  SendDevInfo();
}

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
  resultTempFloat = (float) resultTemp * tGain - tOffset; // Apply calibration correction
  return resultTempFloat;
}

void SendDevInfo()
{
  //========= Temperature =============
  double fTemp = GetTemp();
  
  if (oldTemp != fTemp){
    Serial.print("Temp:");
    Serial.println(fTemp);
  
    if (!send(TempMsg.set(fTemp, 2))){
      oldTemp = 0;
    } else {
      oldTemp = fTemp;
    }
  }

  //========= Battery ============= 
  #ifdef IsMQTT
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
