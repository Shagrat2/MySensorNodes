#define CH1_CURRENT_PIN A6  // Channel 1 ACS712 pin
#define CH2_CURRENT_PIN A7  // Channel 2 ACS712 pin
#define CH1_LIGHT_PIN   3   // Channel 1 RELAY
#define CH2_LIGHT_PIN   4   // Channel 2 RELAY

#define DEB_LED_PIN     5   // Debug led
#define DEB_SWITCH_PIN  6   // Debug switch

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

#include <SPI.h>
#include <MySensors.h>
#include <Bounce2.h>

#ifdef M25P40
	#define SPIFLASH_BLOCKERASE_32K   0xD8
#endif

const long int ACSMin = 505;
const long int ACSMax = 515;

// Instantiate a Bounce object
Bounce debouncer = Bounce(); 

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
  
  // CH1 ACS
  pinMode(CH1_CURRENT_PIN, INPUT);
  Val = analogRead(CH1_CURRENT_PIN);
  if ((Val < ACSMin) || (Val > ACSMax)) {
    Serial.print("ACS error: ");
  } else {
    Serial.print("ACS: OK = ");
  }
  Serial.println(Val);

  // CH2 ACS
  pinMode(CH1_CURRENT_PIN, INPUT);
  Val = analogRead(CH1_CURRENT_PIN);
  if ((Val < ACSMin) || (Val > ACSMax)) {
    Serial.print("ACS error: ");    
  } else {
    Serial.print("ACS: OK = ");    
  }
  Serial.println(Val);

  // Flash
  if (_flash.initialize()) {
    Serial.println("Flash: OK");
  } else {    
    Serial.println("Flash: ERROR ");
  }

  // ATASHA
  testSha204();

  // Key
  Serial.println("Key: Watch tester blinks");

  int CH1_state = LOW;
  int CH2_state = LOW;
  int DEB_LED_state = LOW;
  int DEB_SWITCH_state = LOW;
  
  unsigned long CH1_prev = 0;
  unsigned long CH2_prev = 0;
  unsigned long DEB_prev = 0;

  pinMode(CH1_LIGHT_PIN, OUTPUT);
  pinMode(CH2_LIGHT_PIN, OUTPUT);

  pinMode(DEB_LED_PIN, OUTPUT);
  
  pinMode(DEB_SWITCH_PIN,INPUT_PULLUP);
  debouncer.attach(DEB_SWITCH_PIN);
  debouncer.interval(5); // interval in ms
    
  while (true) {
    unsigned long currentMillis = millis();  
   
    
    if (currentMillis - CH1_prev >= 1000) {
      CH1_prev = currentMillis;
      CH1_state = CH1_state == LOW ? CH1_state = HIGH: CH1_state = LOW;      
      digitalWrite(CH1_LIGHT_PIN, CH1_state);
    }

    if (currentMillis - CH2_prev >= 2000) {
      CH2_prev = currentMillis;
      CH2_state = CH2_state == LOW ? CH2_state = HIGH: CH2_state = LOW;      
      digitalWrite(CH2_LIGHT_PIN, CH2_state);      
    }

    if (currentMillis - DEB_prev >= 500) {
      DEB_prev = currentMillis;
      DEB_LED_state = DEB_LED_state == LOW ? DEB_LED_state = HIGH: DEB_LED_state = LOW;      
      digitalWrite(DEB_LED_PIN, DEB_LED_state);
    }

    debouncer.update();
    int value = debouncer.read();
    if (value != DEB_SWITCH_state) {
      DEB_SWITCH_state = value;
      if (value == LOW) {
        Serial.println("DEB button: UP ");
      } else {
        Serial.println("DEB button: DOWN ");
      }
    }
    
  }
}

void loop() {

}

