#define TEMP_PIN    A3  // Termistor pin
#define CURRENT_PIN A6  // ACS712 pin
#define LIGHT_PIN   3   // Arduino Digital I/O pin number for first relay
#define MY_SIGNING_ATSHA204_PIN 16 // A2

//#DEFINE M25P40 // Flash type

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

#include <SPI.h>
#include <MySensors.h>

#ifdef M25P40
	#define SPIFLASH_BLOCKERASE_32K   0xD8
#endif

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

  Serial.begin(115200);

  // CPU
  Serial.println("Test CPU: OK");

  // NRF24/RFM69
  if (transportInit()) { // transportSanityCheck
    Serial.println("Radio: OK");
  } else {
    Serial.println("Radio: ERROR");
  }

  // NTC
  pinMode(TEMP_PIN, INPUT);
  Val = analogRead(TEMP_PIN);
  if ((Val < 518) || (Val > 542)) {
    Serial.print("NTC error: ");
  } else {
    Serial.print("NTC: OK = ");
  }
  Serial.println(Val);

  // ACS
  pinMode(CURRENT_PIN, INPUT);
  Val = analogRead(CURRENT_PIN);
  if ((Val < 262) || (Val > 269)) {
    Serial.print("ACS error: ");
  } else {
    Serial.print("ACS: OK = ");
  }
  Serial.println(Val);

  // Flash
  if (_flash.initialize()) {
    Serial.println("Flash: OK");
  } else {    
    Serial.print("Flash: ERROR ");
  }

  // ATASHA
  testSha204();

  // Key
  Serial.println("Key: Watch tester blinks");

  pinMode(LIGHT_PIN, OUTPUT);
  while (true) {
    digitalWrite(LIGHT_PIN, HIGH);
    delay(2000);

    digitalWrite(LIGHT_PIN, LOW);
    delay(2000);
  }
}

void loop() {

}

