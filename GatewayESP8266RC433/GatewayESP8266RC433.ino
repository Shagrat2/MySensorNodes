/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * Contribution by a-lurker and Anticimex, 
 * Contribution by Norbert Truchsess <norbert.truchsess@t-online.de>
 * Contribution by Ivo Pullens (ESP8266 support)
 * 
 * DESCRIPTION
 * The EthernetGateway sends data received from sensors to the WiFi link. 
 * The gateway also accepts input on ethernet interface, which is then sent out to the radio network.
 *
 * VERA CONFIGURATION:
 * Enter "ip-number:port" in the ip-field of the Arduino GW device. This will temporarily override any serial configuration for the Vera plugin. 
 * E.g. If you want to use the defualt values in this sketch enter: 192.168.178.66:5003
 *
 * LED purposes:
 * - To use the feature, uncomment WITH_LEDS_BLINKING in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error  
 * 
 * See http://www.mysensors.org/build/esp8266_gateway for wiring instructions.
 * nRF24L01+  ESP8266
 * VCC        VCC
 * CE         GPIO4          
 * CSN/CS     GPIO15
 * SCK        GPIO14
 * MISO       GPIO12
 * MOSI       GPIO13
 * GND        GND
 *            
 * Not all ESP8266 modules have all pins available on their external interface.
 * This code has been tested on an ESP-12 module.
 * The ESP8266 requires a certain pin configuration to download code, and another one to run code:
 * - Connect REST (reset) via 10K pullup resistor to VCC, and via switch to GND ('reset switch')
 * - Connect GPIO15 via 10K pulldown resistor to GND
 * - Connect CH_PD via 10K resistor to VCC
 * - Connect GPIO2 via 10K resistor to VCC
 * - Connect GPIO0 via 10K resistor to VCC, and via switch to GND ('bootload switch')
 * 
  * Inclusion mode button:
 * - Connect GPIO5 via switch to GND ('inclusion switch')
 * 
 * Hardware SHA204 signing is currently not supported!
 *
 * Make sure to fill in your ssid and WiFi password below for ssid & pass.
 */

#include <EEPROM.h>
#include <SPI.h>
#include <RCSwitch.h>

//RC433
unsigned long SEND_FREQUENCY = 30000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway. 
unsigned long RECEIVE_TIMEOUT = 500;

#define RCSENSOR  1
#define RECEIVEPIN 0  // 2 Pin Interupt
#define SENDPID D4
#define TRY_TO_SEND 5

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_GATEWAY_ESP8266

#define MY_ESP8266_SSID "Z-LINK2"
#define MY_ESP8266_PASSWORD "eikvxz8182"

// Enable UDP communication
//#define MY_USE_UDP

// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static.
// #define MY_ESP8266_HOSTNAME "sensor-gateway"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,178,87

// If using static ip you need to define Gateway and Subnet address as well
#define MY_IP_GATEWAY_ADDRESS 10,9,0,254
#define MY_IP_SUBNET_ADDRESS 255,255,255,0

// The port to keep open on node server mode 
#define MY_PORT 5003      

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 5

// Controller ip address. Enables client mode (default is "server" mode). 
// Also enable this if MY_USE_UDP is used and you want sensor data sent somewhere. 
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 68

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE

// Enable Inclusion mode button on gateway
// #define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60 
// Digital pin used for inclusion mode button
#define MY_INCLUSION_MODE_BUTTON_PIN  3 

 
// Flash leds on rx/tx/err
// #define MY_LEDS_BLINKING_FEATURE
// Set blinking period
// #define MY_DEFAULT_LED_BLINK_PERIOD 300

// Led pins used if blinking feature is enabled above
#define MY_DEFAULT_ERR_LED_PIN 16  // Error led pin
#define MY_DEFAULT_RX_LED_PIN  16  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN  16  // the PCB, on board LED

#if defined(MY_USE_UDP)
  #include <WiFiUDP.h>
#else
  #include <ESP8266WiFi.h>
#endif

#include <MySensors.h>

//===============================================
RCSwitch mySwitch = RCSwitch();

uint32_t currentTime; 
uint32_t lastSend;
uint32_t lastRCSend;
uint32_t lastRCId;
uint32_t SendRCId = 0;
uint32_t SendTry = 0;

extern MyMessage _msg;

void setup() { 
  wdt_disable();

  //=== RC
  mySwitch.enableReceive( RECEIVEPIN );
  mySwitch.enableTransmit( SENDPID );    

  wdt_enable(WDTO_8S);  
  
  lastSend=millis();
  lastRCSend=0;
  lastRCId = 0;
}

void presentation() {
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo("GateWay Node witch RC", "2.0");

  present(MY_NODE_ID, S_IR, "RC433");
}

void loop() {
  // Send locally attached sensors data here
  wdt_reset();

  currentTime = millis();

  //==== RC send
  if (SendTry != 0)
  {
    Serial.print("Send RC ");
    Serial.print(SendTry);
    Serial.print(":");
    Serial.println(SendRCId);  

    mySwitch.send( SendRCId, 24 );
    SendTry--;
  } 

  //==== RC receive
  if (mySwitch.available()) {    
    if ((mySwitch.getReceivedBitlength() == 24) && (mySwitch.getReceivedProtocol() == 1))
    {
      uint32_t id = mySwitch.getReceivedValue();
      if ((currentTime - lastRCSend > RECEIVE_TIMEOUT) || (lastRCId != id))
      {
        gatewayTransportSend(build(_msg, GATEWAY_ADDRESS, GATEWAY_ADDRESS, RCSENSOR, C_SET, V_IR_RECEIVE, false).set(id));      
        lastRCSend = currentTime;        
        lastRCId = id;
      }
   }

   Serial.print("Receive ");
   Serial.print(mySwitch.getReceivedValue());
   Serial.print("-");
   Serial.print(mySwitch.getReceivedBitlength());
   Serial.print("bit-P");
   Serial.println(mySwitch.getReceivedProtocol());    
    
   mySwitch.resetAvailable();
 }  
}

void receive(const MyMessage &message) 
{ 
  if (message.isAck()) return;

  if (message.sensor != RCSENSOR) return;    
  if (message.type != V_IR_SEND) return;

  // Set to task
  SendRCId = message.getLong();
  SendTry = TRY_TO_SEND;
}

