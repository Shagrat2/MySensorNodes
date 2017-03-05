                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #define MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FLASH_SS 17
#define MY_OTA_FLASH_JDECID 0x2013

#define MY_SIGNING_ATSHA204_PIN 16 // A2

#define MY_DEBUG
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_CORE_ONLY // Not change
#define MY_SIGNING_ATSHA204

#include <SPI.h>
#include <MySensors.h>

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
  Serial.begin(115200);

  // CPU
  Serial.println("Test CPU: OK");

  // NRF24/RFM69
  if (transportInit()) { // transportSanityCheck
    Serial.println("Radio: OK");
  } else {
    Serial.println("Radio: ERROR");
  }

  // Flash
  SPI.begin(); // инициализация интерфейса SPI
  pinMode(MY_OTA_FLASH_SS, OUTPUT);

  // Read ID
  digitalWrite(MY_OTA_FLASH_SS, LOW);

  SPI.transfer(SPIFLASH_IDREAD);

  uint8_t head = SPI.transfer(0);

  uint16_t jedecid = SPI.transfer(0) << 8;
  jedecid |= SPI.transfer(0);

  digitalWrite(MY_OTA_FLASH_SS, HIGH);

  if ((head != 0x20) || (jedecid != 0x2013)) {
    Serial.print("Flash: ERROR: ");
    Serial.print("H=");
    Serial.print(head);
    Serial.print(" JEDEC=");
    Serial.println(jedecid);
  } else {
    Serial.println("Flash: OK");
  }

  // ATASHA
  testSha204();  
}

void loop() {

}
