#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "config.h"
#include "MSTcpServer.h"
#include "WebServer.h"
#include "eepromutils.h"

/* Set these to your desired credentials. */
char ssId[cSizeOfSaveVsals];
char ssPass[cSizeOfSaveVsals];

char APCSSId[cSizeOfSaveVsals];
char APCSSPass[cSizeOfSaveVsals];

//==================

bool APConnect;
int APLastState = WL_IDLE_STATUS;
int APCStatus;
unsigned long APCLastTime;

#define ConnectTryTimeOut 10000

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
*/

void setup() {
  wdt_disable();
  
	delay(1000);
	Serial.begin(115200);
	Serial.println();

  // Load params
  EEPROM.begin(512);  
  EEPROMReadStr(cAPName, ssId, sizeof(ssId));
  EEPROMReadStr(cAPPass, ssPass, sizeof(ssPass));
  EEPROMReadStr(cConnectAPName, APCSSId, sizeof(APCSSId));
  EEPROMReadStr(cConnectAPPass, APCSSPass, sizeof(APCSSPass));
  EEPROM.end();

  if (ssId[0] == 0x00) {
    strcpy(ssId, WIFI_SSID);
    Serial.println("Set default SS ID");
  }
  if (ssPass[0] == 0x00){
    strcpy(ssPass, WIFI_PASS);
    Serial.println("Set default SS Password");
  }
  if (APCSSId[0] == 0x00) {
    strcpy(APCSSId, CAP_NAME);
    Serial.println("Set default Connect to AP");
  }
  if (APCSSPass[0] == 0x00){
    strcpy(APCSSPass, CAP_PASS);
    Serial.println("Set default Connect to Password");
  }
   
  Serial.printf("AP ID: %s\n", ssId);
  Serial.printf("AP password: %s\n", ssPass);
  Serial.printf("Connect AP: %s\n", APCSSId);
  Serial.printf("Connect AP password: %s\n", APCSSPass);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  Serial.println("Configuring access point...");

  // WIFI
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.softAP(ssId, ssPass);
  
	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);

  Serial.print("MAC: ");
  Serial.println(WiFi.softAPmacAddress());

  // WebServer
  WebServerInit();  

  //=== Config connect AP
  APConnect = strcmp(APCSSId, "") != 0;
  APCStatus = WL_IDLE_STATUS;
  APCLastTime = millis();

  // MySesnor Gate
  MSTcpServerInit();
  
  wdt_enable(WDTO_8S);
}

void loop() {  
  wdt_reset();
   
  // Web Server
  WebServerHandle();

  // Connect to AP
  unsigned long currentTime = millis();
  if (APConnect && (APCStatus == WL_IDLE_STATUS) && (currentTime - APCLastTime > ConnectTryTimeOut)) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(APCSSId);
    
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    APCStatus = WiFi.begin(APCSSId, APCSSPass);
    APCLastTime = currentTime;
  }

  if ((APLastState != WL_CONNECTED) && (WiFi.status() == WL_CONNECTED)){
    printWifiStatus();
    APLastState = WL_CONNECTED;
  }

  // MySesnor Gate
  MSTcpServerHandle();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
