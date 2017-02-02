#define TEMP_PIN    A3  // Termistor pin
#define CURRENT_PIN A6  // ACS712 pin
#define LIGHT_PIN   3   // Arduino Digital I/O pin number for first relay
#define MY_SIGNING_ATSHA204_PIN 16 // A2

#define MY_DEBUG 
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_CORE_ONLY // Not change
#define MY_SIGNING_ATSHA204 

#include <SPI.h>
#include <MySensors.h>

//SPIFlash _flash(MY_OTA_FLASH_SS, MY_OTA_FLASH_JDECID);

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
        for (int i=0; i<9; i++) {
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

  // NTC
  pinMode(TEMP_PIN, INPUT);
  Val = analogRead(TEMP_PIN);
  if ((Val < 518) || (Val > 530)){
    Serial.print("NTC error: ");    
  } else {
    Serial.print("NTC: OK = ");    
  }
  Serial.println(Val);

  // ACS
  pinMode(CURRENT_PIN, INPUT);
  Val = analogRead(CURRENT_PIN);
  if ((Val < 262) || (Val > 266)){
    Serial.print("ACS error: ");    
  } else {
    Serial.print("ACS: OK = ");
  }
  Serial.println(Val);

  // Flash
/*  
  if (!_flash.initialize()) { 
    Serial.println("Flash: ERROR");
  } else {
    Serial.println("Flash: OK");
  }
*/
  // ATASHA
  testSha204();

  // NRF24/RFM69  
  if (transportSanityCheck()) {
    Serial.println("Radio: OK");
  } else {
    Serial.println("Radio: ERROR");
  }
  
  // Key
  Serial.println("Key: Watch tester blinks");
  
  pinMode(LIGHT_PIN, OUTPUT);
  while (true){    
    digitalWrite(LIGHT_PIN, HIGH);
    delay(2000);
  
    digitalWrite(LIGHT_PIN, LOW);
    delay(2000);
  }
}

void loop(){

}

