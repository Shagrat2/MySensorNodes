#define MY_DEBUG 
#define DEBUG // Show debug to serrial

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

#define MY_SIGNING_ATSHA204_PIN 16 // A2

#define M25P40 // Flash type
#define MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FLASH_SS 7

//#define SYSTEM_BATTERY

#define SensBMP180 // 21% -26%
#define SensSI7021 // 18% - 24%
#define SensBH1750 // 13% - 21%
//#define SensDHT // 18% - 22%
//#define SensDS18B20 // 21% - 20%
#define SensKey

// Flash options
#define MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FLASH_SS 7
//#DEFINE M25P40 // Flash type

#ifdef M25P40
  #define MY_OTA_FLASH_JDECID 0x2020
#endif

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <EEPROM.h> 

#define SENS_POW_TIME (60ul)
#define MY_SMART_SLEEP_WAIT_DURATION_MS (200ul)

#ifdef M25P40
  #define SPIFLASH_BLOCKERASE_32K   0xD8
#endif

// BMP180
#ifdef SensBMP180
  #include <Adafruit_BMP085.h>
#endif

// SI7021
#ifdef SensSI7021
  #include "i2c_SI7021.h"
#endif

// BH1750
#ifdef SensBH1750
  #include <BH1750.h>  
#endif

// DHT
#ifdef SensDHT
  #include "DHT.h"
#endif

// DS18B20
#ifdef SensDS18B20
  #include <OneWire.h>
  #include <DallasTemperature.h>
#endif

#define MIN_V 2800 // empty voltage (0%)
#define MAX_V 3700 // full voltage (100%) 

#define SKETCH_NAME "MSMD Multi Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "2"

unsigned long SEND_FREQUENCY = (15*60*1000ul); // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 

#define POWER_PIN   6
#define INIT_PIN    A4
#define LED_PIN     14
#define BATTERY_PIN A7

#define BMP180_ID   1
#define SI7021_ID   2
#define BH1750_ID   3
#define DHT_ID      4
#define KEY_ID      5
#define BATTERY_ID  240
#define DALAS_ID    10

//== BMP180
#ifdef SensBMP180
  Adafruit_BMP085 bmp;
  float BMP180PressLast = 0;
  float BMP180TempLast = 0;

  MyMessage MesBMP180Pres(BMP180_ID, V_PRESSURE);
  MyMessage MesBMP180Temp(BMP180_ID, V_TEMP);
#endif

//== SI7021
#ifdef SensSI7021
  SI7021 si7021;

  float SI7021HumiLast = 0;
  float SI7021TempLast = 0;
  
  MyMessage MesSI7021Humi(SI7021_ID, V_HUM);
  MyMessage MesSI7021Temp(SI7021_ID, V_TEMP);
#endif

//== BH1750
#ifdef SensBH1750
  #define SENS_POW_TIME (200ul)
  
  BH1750 lightMeter(0x23);

  uint16_t BH1750LuxLast = 0;
  MyMessage MesBH1750LuxLux(BH1750_ID, V_LIGHT_LEVEL);
#endif

//== DHT
#ifdef SensDHT
  #define DHTPIN 5     // what digital pin we're connected to
  //#define DHTTYPE DHT11   // DHT 11
  #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
  //#define DHTTYPE DHT21   // DHT 21 (AM2301)

  #if DHTTYPE == DHT11
    #define SENS_POW_TIME (1100ul)
  #endif
  #if DHTTYPE == DHT21
    #define SENS_POW_TIME (1100ul)
  #endif
  #if DHTTYPE == DHT22
    #define SENS_POW_TIME (550ul)
  #endif
  
  DHT dht(DHTPIN, DHTTYPE);
  
  float DHTTempLast = NAN;
  float DHTHumLast = NAN;

  MyMessage MesDHTHumi(DHT_ID, V_HUM);
  MyMessage MesDHTTemp(DHT_ID, V_TEMP);
#endif
  
//== DS18B20
#ifdef SensDS18B20
  #define ONE_WIRE_BUS 4
  #define MAX_ATTACHED_DS18B20 4
  
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);

  float lastTemperature[MAX_ATTACHED_DS18B20];
  int numSensors=0;  

  MyMessage MesDSTemp(DALAS_ID, V_TEMP);
#endif

#ifdef SensKey
  #define KEY_PIN 3

  uint8_t KeyValueLast = 0xFF;
  long KeySendTry=0;
  #define cMaxSmallTry 3

  MyMessage MsgKeyValue(KEY_ID, V_TRIPPED);
#endif

#ifndef SYSTEM_BATTERY
  MyMessage BattMsg(BATTERY_ID, V_VOLTAGE);
#endif
MyMessage BattRaw(BATTERY_ID, V_VAR1);

void before(){
  wdt_disable();

  Serial.begin(115200);
  #ifdef DEBUG
    Serial.println("Start");
  #endif

  // Init adress from pin
  pinMode(INIT_PIN, INPUT); 
  if (analogRead(INIT_PIN) == 0){    
    #ifdef DEBUG
      Serial.println("Init adress");
    #endif
    
    for (int i=0;i<512;i++) {
      EEPROM.write(i, 0xff);
    } 
  }

  // Battery pin
  pinMode(BATTERY_PIN, INPUT);

  // Setup the buttons
  pinMode(POWER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Power to sensors
  digitalWrite(POWER_PIN, LOW); 
  
  #ifdef SensKey
    Serial.println("Key========");
    pinMode(KEY_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(KEY_PIN), onPulse, CHANGE); 
  #endif
 
  #ifdef DEBUG 
    Serial.print("POW time (ul): ");
    Serial.println(SENS_POW_TIME);
  #endif

  //=== BMP
  #ifdef SensBMP180
    if (!bmp.begin()){
      #ifdef DEBUG
        Serial.println("BMP180 - Sensor missing");
      #endif
    }  
  #endif

  //=== si7021
  #ifdef SensSI7021
    if (!si7021.initialize()){
      #ifdef DEBUG
        Serial.println("si7021 - Sensor missing");
      #endif
    }
    
  #endif

  //=== BH1750
  #ifdef SensBH1750
    lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE);

    #ifdef DEBUG
        uint8_t RSize = Wire.requestFrom(0x23, 2);
        if (RSize != 2){
          Serial.println("BH1750 - Sensor missing");  
        } 
        Wire.read(); 
        Wire.read();        
    #endif    
  #endif
  
  //=== DHT  
  #ifdef SensDHT
    dht.begin();
  #endif

  //=== DS18B20
  #ifdef SensDS18B20
    sensors.begin();
    //sensors.setWaitForConversion(false);
    numSensors = sensors.getDeviceCount();
  #endif

  #ifdef MY_OTA_FIRMWARE_FEATURE  
    #ifdef DEBUG
      Serial.println("OTA FW update enabled");
    #endif
  #endif
     
  wdt_enable(WDTO_8S); 
}

void setup(){
  wdt_reset();
  digitalWrite(LED_PIN, HIGH);
}

#ifdef SensKey
void onPulse(){
  Serial.println("OPS");
  
  wait(5);

  // Door sensor
  uint8_t KeyValue = digitalRead(KEY_PIN); 
  if (KeyValue != KeyValueLast) {
     // Value has changed from last transmission, send the updated value
     if (!send(MsgKeyValue.set(KeyValue==HIGH ? 1 : 0))){
       // Not send last state       
       if (KeySendTry < cMaxSmallTry){
         KeySendTry++;
         //errsend++;
       }
     } else {
       KeyValueLast = KeyValue;
       KeySendTry = 0;
     }

     #ifdef DEBUG
       Serial.print("[KEY]:");
       Serial.println(KeyValue);
     #endif
  } 

  smartSleep(50);
}
#endif

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);
  wait(50);

  // Battery
  present(BATTERY_ID, S_MULTIMETER, "Battery");
  wait(50);
  
  // Register sensos
  #ifdef SensBMP180
    present(BMP180_ID, S_BARO, "BMP180 sensor");  
    wait(50);
  #endif
  #ifdef SensSI7021
    present(SI7021_ID, S_HUM, "Si7021 sensor");
    wait(50);
  #endif
  #ifdef SensBH1750
    present(BH1750_ID, S_LIGHT, "BH1750 sensor");
    wait(50);  
  #endif 
  #ifdef SensDHT
    present(DHT_ID, S_HUM, "DHT sensor");
    wait(50);
  #endif 
  #ifdef SensDS18B20
    present(DALAS_ID, S_TEMP, "DS sensors");
    wait(50);
  #endif 
  #ifdef SensKey
    present(KEY_ID, S_BINARY, "Key sensors");
    wait(50);
  #endif 
}

long readVcc() {
  return analogRead(BATTERY_PIN) * 3.7; // 3.616813294232649;
 /* 
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
  */
}

void SendDevInfo()
{
  //========= Battery ============= 
  #ifdef SYSTEM_BATTERY
    int batteryV = readVcc();
    int batteryPcnt = min(map(batteryV, MIN_V, MAX_V, 0, 100), 100);
    sendBatteryLevel( batteryPcnt ); 
    
    Serial.print("BatV:");
    Serial.println(batteryV * 0.001);        
  #else
    float batteryV  = readVcc() * 0.001;
    send(BattMsg.set(batteryV, 2));    
    
    Serial.print("BatV:");
    Serial.println(batteryV);  
  #endif
  
  #ifdef SendSkipTry
    if (lasterrsend != errsend){
      send(ErrSend.set(errsend));
      lasterrsend = errsend;
    }
  #endif
}
  
void loop() {  
  wdt_reset();

  // Power on
  #ifdef DEBUG     
     Serial.println("***********");
     Serial.print("Wakeup: ");
     Serial.println(millis());

    digitalWrite(LED_PIN, LOW);
  #endif
      
  digitalWrite(POWER_PIN, LOW);
  wait(SENS_POW_TIME);
  wdt_reset();

  #ifdef DEBUG     
     Serial.print("Read: ");
     Serial.println(millis());
  #endif
  
  // === Read values  
  #ifdef SensBMP180    
    float BMP180Press = bmp.readPressure() / 133.3;    
    float BMP180Temp = bmp.readTemperature();    
  #endif

  #ifdef SensSI7021
    static float SI7021Humi, SI7021Temp;
    si7021.getHumidity(SI7021Humi);    
    si7021.getTemperature(SI7021Temp);    
    si7021.triggerMeasurement();    
  #endif

  #ifdef SensBH1750
    uint16_t BH1750Lux = lightMeter.readLightLevel();    
  #endif
  
  #ifdef SensDHT    
    float DHTHum = dht.readHumidity();
    float DHTTemp = dht.readTemperature();
  #endif

  #ifdef SensDS18B20
     // Fetch temperatures from Dallas sensors
     sensors.requestTemperatures();
    
     // query conversion time and sleep until conversion completed
     //int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
     // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
     //@@@ wait(conversionTime);
     //delay(conversionTime);     
    
     // Read temperatures and send them to controller 
     float DSTemp[MAX_ATTACHED_DS18B20];
     for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
       // Fetch and round temperature to one decimal
       DSTemp[i] = (sensors.getTempCByIndex(i) * 10.) / 10.;
     }
  #endif

  // Power off
  digitalWrite(POWER_PIN, HIGH);

  #ifdef DEBUG     
     Serial.print("Send: ");
     Serial.println(millis());
  #endif

  //== Send  value
  #ifdef SensBMP180
    // Press    
    if (BMP180Press != BMP180PressLast) {
      send(MesBMP180Pres.set(BMP180Press, 2));
      #ifdef DEBUG
         Serial.print("[BMP180] Press:");
         Serial.println(BMP180Press);
      #endif
      BMP180PressLast = BMP180Press;
    }
    // Temp    
    if (BMP180Temp != BMP180TempLast) {
      send(MesBMP180Temp.set(BMP180Temp, 2));
      #ifdef DEBUG
         Serial.print("[BMP180] Temp:");
         Serial.println(BMP180Temp);
      #endif
      BMP180TempLast = BMP180Temp;
    } 
  #endif

  #ifdef SensSI7021
    if (SI7021Humi != SI7021HumiLast) {
      send(MesSI7021Humi.set(SI7021Humi, 2));
      #ifdef DEBUG
        Serial.print("[Si7021] HUMI: ");
        Serial.println(SI7021Humi);
      #endif
      SI7021HumiLast = SI7021Humi;
    }
    
    if (SI7021Temp != SI7021TempLast) {
      send(MesSI7021Temp.set(SI7021Temp, 2));
      #ifdef DEBUG
        Serial.print("[Si7021] TEMP: ");
        Serial.println(SI7021Temp);
      #endif  
      SI7021TempLast = SI7021Temp;
    }
  #endif

  #ifdef SensBH1750    
    if (BH1750Lux != BH1750LuxLast){
      send(MesBH1750LuxLux.set(BH1750Lux));  
      #ifdef DEBUG
        Serial.print("[BH1750] Light: ");
        Serial.print(BH1750Lux);
        Serial.println(" lx");
      #endif
      BH1750LuxLast = BH1750Lux;
    }
  #endif

  #ifdef SensDHT    
    // Read Hum
    if ((!isnan(DHTHum)) && (DHTHum != DHTHumLast)) {
      send(MesDHTHumi.set(DHTHum, 0));
      
      #ifdef DEBUG
        Serial.print("[DHT] Humidity: ");
        Serial.print(DHTHum);
        Serial.println(" %\t");
      #endif

      DHTHumLast = DHTHum;
    }
   
    // Read temperature 
    if ((!isnan(DHTTemp)) && (DHTTemp != DHTTempLast)) {      
      send(MesDHTTemp.set(DHTTemp, 2));
      
      #ifdef DEBUG
        Serial.print("[DHT] Temperature: ");
        Serial.print(DHTTemp);
        Serial.println(" *C ");
      #endif

      DHTTempLast = DHTTemp;
    }
  #endif

  //=== DS18B20
  #ifdef SensDS18B20
     for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {              
       if (DSTemp[i] != lastTemperature[i]) {
         MesDSTemp.setSensor(DALAS_ID+i);
         send(MesDSTemp.set(DSTemp[i], 2));
        
         #ifdef DEBUG
           Serial.print("[DS18B20] Temperature D");
           Serial.print(i);
           Serial.print(":");
           Serial.print(DSTemp[i]);
           Serial.println(" *C ");
         #endif       

         lastTemperature[i] = DSTemp[i];
       }
     }
  #endif
  
  #ifdef DEBUG     
     digitalWrite(LED_PIN, HIGH);
     Serial.print("Sleep: ");
     Serial.println(millis());
  #endif

  // wait(SEND_FREQUENCY);
  if (smartSleep(SEND_FREQUENCY) == -1){
     SendDevInfo();
  }
}

void receive(const MyMessage &message) {
  uint8_t Dest = message.sender;

  // Request
  if (mGetCommand(message) == C_REQ) {
    switch (message.sensor) {
      case BATTERY_ID:
         switch (message.type) {
            case V_VAR1:
              int Val = analogRead(BATTERY_PIN);
              send(BattRaw.set(Val));

              break;
         }

         break;
    }
  }
}
