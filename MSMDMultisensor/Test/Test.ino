#define MY_SIGNING_ATSHA204_PIN 16 // A2

#define M25P40 // Flash type

#define MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FLASH_SS 7

#define MY_DEBUG

#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
#define MY_RFM69_FREQUENCY RF69_433MHZ

#define MY_CORE_ONLY // Not change
#define MY_SIGNING_ATSHA204

#ifdef M25P40
	#define MY_OTA_FLASH_JDECID 0x2020
#endif

#define SENS_POWER 6

#include <SPI.h>
#include <MySensors.h>

#include <Wire.h>
#include <Adafruit_BMP085.h>

#include "DHT.h"

#include <OneWire.h> 
#include <DallasTemperature.h>

#ifdef M25P40
	#define SPIFLASH_BLOCKERASE_32K   0xD8
#endif

// BH1750
#define BH1750ADR 0x23
#define BH1750_CONTINUOUS_HIGH_RES_MODE  0x10

// Si7021
#define Si7021ADDR 0x40

// DHT
#define DHTPIN 5
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Dallas
#define ONE_WIRE_BUS 4

bool testSha204()
{
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;
  if (Serial) {
    Serial.print("SHA204: ");
  }
  atsha204_init(MY_SIGNING_ATSHA204_PIN);
  ret_code = atsha204_wakeup(rx_buffer);

  if (ret_code == SHA204_SUCCESS) {
    ret_code = atsha204_getSerialNumber(rx_buffer);
    if (ret_code != SHA204_SUCCESS) {
      if (Serial) {
        Serial.println(F("Failed to obtain device serial number. Response: "));
      }
      Serial.println(ret_code, HEX);
    } else {
      if (Serial) {
        Serial.print(F("OK (serial : "));
        for (int i = 0; i < 9; i++) {
          if (rx_buffer[i] < 0x10) {
            Serial.print('0'); // Because Serial.print does not 0-pad HEX
          }
          Serial.print(rx_buffer[i], HEX);
        }
        Serial.println(")");
      }
      return true;
    }
  } else {
    if (Serial) {
      Serial.println(F("Failed to wakeup SHA204"));
    }
  }
  return false;
}

void setup() {
  int Val;
  uint8_t NBytes;

  Serial.begin(115200);

  // CPU
  Serial.println("Test CPU: OK");

  // NRF24/RFM69
  if (transportInit()) { // transportSanityCheck
    Serial.println("Radio: OK");
  } else {
    Serial.println("Radio: ERROR");
  }

  // Init transaction
  stInitTransition();
  Serial.print("nodeId: ");       Serial.print(getNodeId());        Serial.println("");
  Serial.print("parentNodeId: "); Serial.print(getParentNodeId());  Serial.println("");
  Serial.print("distanceGW: ");   Serial.print(getDistanceGW());    Serial.println("");
  
  // Flash
  if (_flash.initialize()) {
    Serial.println("Flash: OK");
  } else {    
    Serial.println("Flash: ERROR ");
  }

  // ATASHA
  testSha204();

  //--- On power on sesnors
  pinMode(SENS_POWER, OUTPUT);
  digitalWrite(SENS_POWER, LOW);

  Wire.begin();

  //-- BMP180
  Adafruit_BMP085 bmp = Adafruit_BMP085();
  if (!bmp.begin()) {
      Serial.println("BMP180 - ERROR");
  } else {
    delay(500);
    
    Serial.print("BMP180: Temp=");
    Serial.print(bmp.readTemperature());
    Serial.print(" *C    ");
    Serial.print("Pres=");
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");
  }

  //-- Si7021
  uint16_t _ret;
  Wire.beginTransmission(Si7021ADDR);
  Wire.write(0xE5);
  Wire.endTransmission();               // difference to normal read
  Wire.beginTransmission(Si7021ADDR);   // difference to normal read
  NBytes = Wire.requestFrom(Si7021ADDR, (uint8_t) 2);
  if (NBytes != 2) {
    Serial.println("Si7021 - ERROR");
  } else {
    _ret      = (Wire.read() << 8 );
    _ret      |= Wire.read();    

    Serial.print("Si7021: Hum=");
    Serial.println((_ret*125.0/65536) - 6);
  } 
  Wire.endTransmission();

  //-- BH1750
  // Send mode to sensor
  Wire.beginTransmission(BH1750ADR);
  Wire.write((uint8_t)BH1750_CONTINUOUS_HIGH_RES_MODE);
  Wire.endTransmission();

  // Read two bytes from sensor
  uint16_t level;
  Wire.beginTransmission(BH1750ADR);
  NBytes = Wire.requestFrom(BH1750ADR, 2); 
  if (NBytes != 2) {
    Serial.println("BH1750 - ERROR");
  } else {
    // Read two bytes, which are low and high parts of sensor value
    level = Wire.read();
    level <<= 8;
    level |= Wire.read();

    // Convert raw value to lux
    level /= 1.2;

    Serial.print("BH1750: Level=");
    Serial.println(level);
  }
  Wire.endTransmission();

  //-- DHT-11 / DHT-22
  DHT dht(DHTPIN, DHTTYPE);
  dht.begin();
  delay(2000);
  float Temp = dht.readTemperature();
  if (Temp == NAN){
    Serial.println("DHT - ERROR");
  } else {
    Serial.print("DHT: Temp=");
    Serial.println(Temp);
  }

  //-- DS18B20
  OneWire oneWire = OneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);

  sensors.begin();
  int DSCnt = sensors.getDeviceCount();
  if (DSCnt == 0){
    Serial.println("DS - Not found");
  } else {
    Serial.print("DS - found: ");
    Serial.println(DSCnt);
  }    

  digitalWrite(SENS_POWER, HIGH);
}

void loop() {

}

