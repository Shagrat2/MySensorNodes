// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO 

#include <SPI.h>
//@@@ #include <MySensors.h>
#include <avr/wdt.h>

#include "PCF8574.h"

#define SKETCH_NAME "MSMD buttons"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

unsigned long SEND_FREQUENCY = 5000;//900000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway.

PCF8574 pcf;

uint8_t LastN = 0;
uint8_t V1 = 0;
uint8_t V2 = 0;
uint8_t V3 = 0;
uint8_t V4 = 0;
unsigned long mc = 0;
unsigned long LastMic = 0;
unsigned long LastBut = 0;

void setup(){   
  wdt_disable();

  Serial.begin(115200);
  Serial.println("Start"); 
 
  // Disable the ADC by setting the ADEN bit (bit 7) to zero.
  ADCSRA = ADCSRA & B01111111;
  
  // Disable the analog comparator by setting the ACD bit
  // (bit 7) to one.
  ACSR = B10000000; 

  // Подключаем PCFку, задаем адрес I2C
  pcf.begin(0x20);

  Serial.println("Begin"); 

  // Устанавливаем направление пинов
  pcf.pinMode(0, OUTPUT); // Выход
  pcf.pinMode(1, OUTPUT); // Выход
  pcf.pinMode(2, OUTPUT); // Выход
  pcf.pinMode(3, OUTPUT); // Выход
  pcf.pinMode(4, INPUT); // Вход
  pcf.pinMode(5, INPUT); // Вход
  pcf.pinMode(6, INPUT); // Вход
  pcf.pinMode(7, INPUT); // Вход

  // Функция pcf.clear(); выставляет на всех "OUTPUT" портах логический ноль
  pcf.clear();
  delay(400);

/*  
  // Функция pcf.set(); выставляет на всех "OUTPUT" портах логическую единицу
  pcf.set();
  delay(400);

  // Функция pcf.digitalWrite(pin, LOW); выставляет логический ноль на выбранном выводе
  pcf.digitalWrite(3, LOW);
  delay(400);
  // Функция pcf.digitalWrite(pin, HIGH); выставляет логическую единицу на выбранном выводе
  pcf.digitalWrite(3, HIGH);
  delay(400);

  // Функция моргания pcf.blink(pin, количество морганий, промежуток времени);
  pcf.blink(0, 5, 1500);
  delay(200);
  pcf.blink(1, 5, 1000);
  delay(200);
  pcf.blink(2, 5, 500);
  delay(200);
*/

  Serial.println("Ok"); 

  attachInterrupt(digitalPinToInterrupt(2), OnPulse, FALLING );
  pcf.attachInterrupt(4, OnB1, FALLING);
  pcf.attachInterrupt(5, OnB2, FALLING);
  pcf.attachInterrupt(6, OnB3, FALLING);
  pcf.attachInterrupt(7, OnB4, FALLING);

  wdt_enable(WDTO_8S);
}

void OnPulse() {  
    pcf.checkForInterrupt();
    //delay(50);
}

void OnB1() { 
  mc = micros();
  LastBut =1;
}

void OnB2() { 
  mc = micros(); 
  LastBut =2;
}

void OnB3() {  
  mc = micros();
  LastBut =3;
}

void OnB4() {  
  mc = micros();
  LastBut =4;
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
//@@@  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
}

// Loop will iterate on changes on the BUTTON_PINs
void loop() 
{
  wdt_reset();

  if (mc != LastMic) {
    LastMic = mc;  
    
    Serial.print(mc);
    Serial.print(" - ");    
    Serial.println(LastBut);
  }

  
/*
  Serial.println("Off"); 

  pcf.clear();
  //pcf.digitalWrite(0, LOW);
  delay(1000);

  wdt_reset();
  Serial.println("On"); 
  
  // Функция pcf.digitalWrite(pin, HIGH); выставляет логическую единицу на выбранном выводе
  //pcf.digitalWrite(0, HIGH);
  pcf.set();
  delay(1000);

  uint8_t Val = pcf.digitalRead(4);
  if (Val != last) {
    last = Val;
    unsigned long mic = micros();

    Serial.print("Change:"); 
    Serial.print(mic);
    Serial.print(" - ");
    Serial.println(Val);
  }
*/  
}
