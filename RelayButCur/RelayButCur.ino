// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69 

#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

#include <SPI.h>
#include <MySensors.h>
#include <avr/wdt.h>
#include <EEPROM.h> 
#include <Bounce2.h>

#define SKETCH_NAME "Relay Button Current"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

#define CURRENT_PIN A0
#define BUTTON_PIN 2

#define RELAY_PIN 3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON  1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

#define CHILD_ID     1
#define CHILD_ID_DEV 254

unsigned long SEND_FREQUENCY = (15*60*1000); // 15 min, Minimum time between send (in milliseconds). We don't wnat to spam the gateway.  

int countvalues = 50;      // how many values must be averaged
float sensorValue = 0;  // variable to store the value coming from the sensor

float tGain = 0.9338;
float tOffset = 208;

int oldValue=-1;
int oldCurrentValue = 0;

MyMessage StateMsg(CHILD_ID, V_LIGHT);
MyMessage TempMsg(CHILD_ID_DEV, V_TEMP);
MyMessage CurMsg(CHILD_ID_DEV, V_CURRENT);

Bounce debouncer = Bounce(); 

unsigned long currentTime; 
unsigned long lastSend;

double fOldTemp;
int ZerroValue = 0;

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

int Current(int sensorPin) {
  float TMPsensorValue = 0;
  int count = 0;
 
  for (count =0; count < countvalues; count++) {
    // read the value from the sensor:
    TMPsensorValue = analogRead(sensorPin);
    delay(30);
    // make average value
    sensorValue = (sensorValue+TMPsensorValue)/2;
    }
  return sensorValue;
}
  
void SendDevInfo()
{
  //========= Temperature =============
  double fTemp = GetTemp();
  if (fTemp != fOldTemp){
    Serial.print("Temp:");
    Serial.println(fTemp);
  
    send(TempMsg.set(fTemp, 2));
    fOldTemp = fTemp;
  }

  int CurrentValue = Current( CURRENT_PIN );
  if (CurrentValue != oldCurrentValue){
    Serial.print("Cur:");
    Serial.println(CurrentValue);
    
    send(CurMsg.set(CurrentValue));
    oldCurrentValue = CurrentValue;
  }
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
  
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN, HIGH);

  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);

  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);   
  
  // Set relay to last known state (using eeprom storage) 
  digitalWrite(CHILD_ID, loadState(CHILD_ID)?RELAY_ON:RELAY_OFF);

  fOldTemp = -9999;
  
  wdt_enable(WDTO_8S);
}

void setup(){
}

void  presentation() { 
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);
  
  present(CHILD_ID, S_LIGHT, "Relay actuator");    
}

void loop() 
{
  wdt_reset();
   
  // Get the update value
  debouncer.update();
  int value = debouncer.read();
 
  if (value != oldValue) {         
         
     if (value == true){
       // Change relay state
       int state = loadState(CHILD_ID)?RELAY_OFF:RELAY_ON;

       // Set state
       digitalWrite(RELAY_PIN, state);
     
       // Store state in eeprom
       saveState(CHILD_ID, state);

       // Send new value
       send(StateMsg.set(state), true);       
     }

     oldValue = value;
  }

  // Send feq
  // Only send values at a maximum frequency or woken up from sleep
  currentTime = millis();
  bool sendTime = currentTime - lastSend > SEND_FREQUENCY;  
  if (sendTime) {
    SendDevInfo();
    lastSend=currentTime;
  }   
}

void incomingMessage(const MyMessage &message) {  
  
  // We only expect one type of message from controller. But we better check anyway.
  if (!message.isAck() && (message.type==V_LIGHT)) {
     // Change relay state
     digitalWrite(RELAY_PIN, message.getBool()?RELAY_ON:RELAY_OFF);
     
     // Store state in eeprom
     saveState(CHILD_ID, message.getBool());
     
     // Write some debug info     
     Serial.print("Incoming change for sensor:");     
     Serial.println(message.getBool());
   } 
}

