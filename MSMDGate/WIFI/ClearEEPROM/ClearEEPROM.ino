#include <EEPROM.h>

void setup() { 
  delay(1000);
  Serial.begin(115200);
  Serial.println(); 

  EEPROM.begin(512);  
  for (int i=0; i < 512; i++){
     EEPROM.write(i, 0x00);
  }
  EEPROM.end();  

  Serial.println("Clear ok");
}

void loop() {  

}

