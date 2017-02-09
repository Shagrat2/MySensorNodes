#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* Set these to your desired credentials. */
const char *ssid = "MSMDAP";
const char *password = "";

char APCSSId[32] = "Z-LINK2";
char APCPass[32] = "eikvxz8182";

#define MAX_SRV_CLIENTS 4

//==================

ESP8266WebServer WebSrv(80);
WiFiServer MYSSrv(5003);
WiFiClient MYSSrvClients[MAX_SRV_CLIENTS];
ESP8266HTTPUpdateServer httpUpdater;

const char* host = "MSMDGate";
const char* update_path = "/firmware";

const char* FirstPage =
  "<h1>You are connected</h1>"
  "<a href=\"/firmware\">Update firmware</a>";

bool APConnect;
int APCStatus;
unsigned long APCLastTime;

#define ConnectTryTimeOut 10000

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleRoot() {
	WebSrv.send(200, "text/html", FirstPage);
}

void setup() {
  wdt_disable();
  
	delay(1000);
	Serial.begin(115200);
	Serial.println();

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  Serial.print("Configuring access point...");
  
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	
  //== Update
  MDNS.begin(host);
  httpUpdater.setup(&WebSrv, update_path);  

  //== Web server
  WebSrv.on("/", handleRoot);
  WebSrv.begin();
  Serial.println("HTTP server started"); 

  MDNS.addService("http", "tcp", 80);

  //=== Config connect AP
  APConnect = true;
  APCStatus = WL_IDLE_STATUS;
  APCLastTime = millis();

  //== MySensor server
  MYSSrv.begin();
  MYSSrv.setNoDelay(true);

  wdt_enable(WDTO_8S);
}

void loop() {  
  wdt_reset();
  
  uint8_t i;
  
  // Web Server
	WebSrv.handleClient();  

  // Connect to AP
  unsigned long currentTime = millis();
  if (APConnect && (APCStatus == WL_IDLE_STATUS) && (currentTime - APCLastTime > ConnectTryTimeOut)) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(APCSSId);
    
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    APCStatus = WiFi.begin(APCSSId, APCPass);
    APCLastTime = currentTime;
    printWifiStatus();
  }

  // MYS server check connection new clients
  if (MYSSrv.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!MYSSrvClients[i] || !MYSSrvClients[i].connected()){
        if(MYSSrvClients[i]) MYSSrvClients[i].stop();
        MYSSrvClients[i] = MYSSrv.available();
        Serial1.print("New client: "); Serial1.print(i);
        continue;
      }
    }
    
    //no free/disconnected spot so reject
    WiFiClient serverClient = MYSSrv.available();
    serverClient.stop();
  }

  // MYS check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (MYSSrvClients[i] && MYSSrvClients[i].connected()){
      if(MYSSrvClients[i].available()){
        //get data from the telnet client and push it to the UART
        while(MYSSrvClients[i].available()) {
          Serial.write(MYSSrvClients[i].read());
        }
      }
    }
  }

  //check UART for data
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (MYSSrvClients[i] && MYSSrvClients[i].connected()){
        MYSSrvClients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
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
